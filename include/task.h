//实现一个链式调用的任务链功能，来减少callback hell的问题
//通过std::future, 获取最后一个调用的结果。如果中途发生了异常，则std::future::get()会抛出异常
//支持将后续调用指定到特定的“执行环境”上去调用，从而控制执行的时机和线程
//支持C++14/17。由于想节省一些内存，使用了VS的_Promise实现，故只支持VS2015/VS2017

#pragma once

#pragma warning(disable : 4503)			//任务链很容易类型名过长，所以，禁止这个警告

#include <assert.h>
#include <tuple>
#include <future>
#include <functional>

#include "task_utils.inl"
#include "task_invoke_traits.h"
#include "task_detail.inl"
#include "task_executor.inl"
#include "task_exception.inl"
#include "task_node.inl"
#include "task_cbnode.inl"

namespace lib_shark_task
{
	constexpr auto _cb = std::placeholders::_1;
	//using _ = decltype(std::ignore);

	template<class _LastNode, class _FirstNode>
	struct task
	{
		using node_type = _FirstNode;
		using node_type_sptr = std::shared_ptr<node_type>;

		using last_node = _LastNode;
		using last_type = typename last_node::result_type;
	private:
		node_type_sptr						_Node;				//第一个任务节点。其后的then/marshal，这个节点是不变化的
		std::shared_ptr<last_node>			_Last;				//最后一个任务节点。每次then/marshal后，新的task的这个类型和值发生变化
		task_set_exception_agent_sptr		_Exception;			//传递异常的代理接口
		mutable std::atomic<bool>			_Node_executed;		//_Node是否已经执行过了
	public:
		task(const task_set_exception_agent_sptr & exp, const std::shared_ptr<last_node> & last, const std::shared_ptr<node_type> & first)
			: _Node(first)
			, _Last(last)
			, _Exception(exp)
			, _Node_executed(false)
		{
			assert(_Node != nullptr);
			assert(_Last != nullptr);
		}

		task(task && _Right)
			: _Node(std::move(_Right._Node))
			, _Last(std::move(_Right._Last))
			, _Exception(std::move(_Right._Exception))
			, _Node_executed(_Right._Node_executed.load())
		{
		}

		task & operator = (task && _Right)
		{
			if (this != &_Right)
			{
				_Node = std::move(_Right._Node);
				_Last = std::move(_Right._Last);
				_Exception = std::move(_Right._Exception);
				_Node_executed = _Right._Node_executed.load();
			}
			return *this;
		}
		task(const task & st) = delete;
		task & operator = (const task & st) = delete;

		//开始执行任务链的第一个节点。
		//执行完毕后，将第一个任务节点设为nullptr。这样就不能再次调用了（划掉）
		//执行完毕后，如果不保存_Node，则when_all/when_any里会出现提前被析构的风险。
		//			故后来修改为使用_Node_executed来标记是否执行过，从而实现只能执行一次的逻辑
		template<class... _Args>
		void operator()(_Args&&... args) const
		{
			if (_Node_executed)
				throw std::future_error(std::make_error_code(std::future_errc::promise_already_satisfied));

			_Node_executed = true;
			if (_Node->invoke_thiz(std::forward<_Args>(args)...))
				_Node->invoke_then_if();
		}

		//获取最后一个任务节点，对应的future。
		inline auto get_future()
		{
			assert(!_Last->is_retrieved());
			return _Last->get_future();
		}

		//执行器只能获取一次。
		//获取完毕后，自身不能再调用operator()了----毕竟是run_once语义
		inline auto get_executor()
		{
			if (_Node_executed)
				throw std::future_error(std::make_error_code(std::future_errc::promise_already_satisfied));

			_Node_executed = true;
			return std::make_shared<task_executor<node_type>>(std::move(_Node));
		}

		//根据下一个任务节点，生成新的任务链对象
		template<class _Ftype>
		inline auto then(_Ftype && fn)
		{
			assert(_Last != nullptr);
			assert(!_Last->is_retrieved());

			using then_result = detail::result_of_t<std::remove_reference_t<_Ftype>>;
			using args_tuple_type = detail::args_tuple_t<std::remove_reference_t<_Ftype>>;
			using last_tuple_type = detail::package_tuple_t<last_type>;

			static_assert(std::tuple_size<args_tuple_type>::value <= std::tuple_size<last_tuple_type>::value, "parames of '_Ftype' is not match this task node(s).");

			using next_node_type = detail::unpack_tuple_node_t<then_result, args_tuple_type/*last_type*/>;

			auto st_next = std::make_shared<next_node_type>(fn, _Exception);
			_Exception->_Impl = st_next.get();
			_Last->_Set_then_if(detail::_Set_then_helper<next_node_type>{ st_next });

			return task<next_node_type, node_type>{_Exception, st_next, _Node};
		}

		//根据下一个任务节点，生成新的任务链对象
		//指定此任务节点必须在ctx所代表的线程/时机里运行
		template<class _Context, class _Ftype>
		inline auto then(_Context & ctx, _Ftype && fn)
		{
			assert(_Last != nullptr);
			assert(!_Last->is_retrieved());

			using then_result = detail::result_of_t<std::remove_reference_t<_Ftype>>;
			using args_tuple_type = detail::args_tuple_t<std::remove_reference_t<_Ftype>>;
			using last_tuple_type = detail::package_tuple_t<last_type>;

			static_assert(std::tuple_size<args_tuple_type>::value <= std::tuple_size<last_tuple_type>::value, "");

			using next_node_type = detail::unpack_tuple_node_t<then_result, args_tuple_type/*last_type*/>;

			auto st_next = std::make_shared<next_node_type>(std::forward<_Ftype>(fn), _Exception);
			_Exception->_Impl = st_next.get();
			_Last->_Set_then_if(detail::_Set_then_ctx_helper<_Context, next_node_type>{ &ctx, st_next });

			return task<next_node_type, node_type>{_Exception, st_next, _Node};
		}

		template<class _Fcb, class... _Types>
		inline auto marshal(_Fcb&& fn, _Types&&... args)
		{
			assert(_Last != nullptr);
			assert(!_Last->is_retrieved());

			using callback_type3 = detail::args_of_t<detail::get_holder_index<_Types...>::index, _Fcb>;
			using callback_type2 = std::remove_cv_t<std::remove_reference_t<callback_type3>>;
			using callback_type = typename detail::invoke_traits<callback_type2>::type;

			using next_node_type = detail::unpack_tuple_cbnode_t<callback_type, last_type>;

			auto st_next = std::make_shared<next_node_type>(_Exception, std::forward<_Fcb>(fn), std::forward<_Types>(args)...);
			_Exception->_Impl = st_next.get();
			_Last->_Set_then_if(detail::_Set_then_helper<next_node_type>{ st_next });

			return task<next_node_type, node_type>{_Exception, st_next, _Node};
		}

		template<class _Context, class _Fcb, class... _Types>
		inline auto marshal(_Context & ctx, _Fcb&& fn, _Types&&... args)
		{
			assert(_Last != nullptr);
			assert(!_Last->is_retrieved());

			using callback_type3 = detail::args_of_t<detail::get_holder_index<_Types...>::index, _Fcb>;
			using callback_type2 = std::remove_cv_t<std::remove_reference_t<callback_type3>>;
			using callback_type = typename detail::invoke_traits<callback_type2>::type;

			using next_node_type = detail::unpack_tuple_cbnode_t<callback_type, last_type>;

			auto st_next = std::make_shared<next_node_type>(_Exception, std::forward<_Fcb>(fn), std::forward<_Types>(args)...);
			_Exception->_Impl = st_next.get();
			_Last->_Set_then_if(detail::_Set_then_ctx_helper<_Context, next_node_type>{ &ctx, st_next });

			return task<next_node_type, node_type>{_Exception, st_next, _Node};
		}

		inline task_set_exception_agent_sptr & _Get_exception_agent()
		{
			return _Exception;
		}

		template<class _Nnode>
		inline auto _Then_node(const std::shared_ptr<_Nnode> & st_next)
		{
			assert(_Last != nullptr);
			assert(!_Last->is_retrieved());
			using next_node_type = std::remove_reference_t<_Nnode>;

			_Last->_Set_then_if(detail::_Set_then_helper<next_node_type>{ st_next });
			return task<next_node_type, node_type>{_Exception, st_next, _Node};
		}

		template<class _Context, class _Nnode>
		inline auto _Then_node(_Context & ctx, const std::shared_ptr<_Nnode> & st_next)
		{
			assert(_Last != nullptr);
			assert(!_Last->is_retrieved());
			using next_node_type = std::remove_reference_t<_Nnode>;

			_Last->_Set_then_if(detail::_Set_then_ctx_helper<_Context, next_node_type>{ &ctx, st_next });
			return task<next_node_type, node_type>{_Exception, st_next, _Node};
		}
	};

	namespace detail
	{
		template<class _Ty>
		struct is_task2 : public std::false_type {};
		template<class _LastNode, class _FirstNode>
		struct is_task2<task<_LastNode, _FirstNode>> : public std::true_type {};
		template<class _Ty>
		struct is_task : public is_task2<typename std::remove_reference<_Ty>::type> {};
	}

	template<size_t _ArgSize>
	struct _Make_task_0_impl
	{
		template<class _Ftype>
		static inline auto make(_Ftype && fn)
		{
			using fun_type_rrt = std::remove_reference_t<_Ftype>;
			using ret_type = detail::result_of_t<fun_type_rrt>;
			using args_tuple_type = typename detail::invoke_traits<fun_type_rrt>::args_tuple_type;

			using ret_type_rrt = std::remove_reference_t<ret_type>;
			using first_node_type = detail::unpack_tuple_node_t<ret_type_rrt, args_tuple_type>;

			task_set_exception_agent_sptr exp = std::make_shared<task_set_exception_agent>();
			auto st_next = std::make_shared<first_node_type>(std::forward<_Ftype>(fn), exp);
			exp->_Impl = st_next.get();

			return task<first_node_type, first_node_type>{exp, st_next, st_next};
		}
	};
	template<>
	struct _Make_task_0_impl<0>
	{
		template<class _Ftype>
		static inline auto make(_Ftype && fn)
		{
			using fun_type_rrt = std::remove_reference_t<_Ftype>;
			using ret_type = detail::result_of_t<fun_type_rrt>;
			using args_tuple_type = typename detail::invoke_traits<fun_type_rrt>::args_tuple_type;

			using ret_type_rrt = std::remove_reference_t<ret_type>;
			using first_node_type = task_node<ret_type_rrt>;

			task_set_exception_agent_sptr exp = std::make_shared<task_set_exception_agent>();
			auto st_next = std::make_shared<first_node_type>(std::forward<_Ftype>(fn), exp);
			exp->_Impl = st_next.get();

			return task<first_node_type, first_node_type>{exp, st_next, st_next};
		}
	};

	//根据一个新的任务节点，生成一个全新的任务链
	template<class _Ftype>
	inline auto make_task(_Ftype && fn)
	{
		using fun_type_rrt = std::remove_reference_t<_Ftype>;
		using args_tuple_type = typename detail::invoke_traits<fun_type_rrt>::args_tuple_type;

		return _Make_task_0_impl<std::tuple_size<args_tuple_type>::value>::make(std::forward<_Ftype>(fn));
	}
	inline auto make_task()
	{
		return make_task(detail::dummy_method_{});
	}
	template<class _Context, class _Ftype>
	inline auto make_task(_Context & ctx, _Ftype && fn)
	{
		return make_task().then(ctx, std::forward<_Ftype>(fn));
	}

	template<class _Fcb, class... _Args>
	inline auto marshal_task(_Fcb&& fn, _Args&&... args)
	{
		using fun_type_rrt = std::remove_reference_t<_Fcb>;
		using args_tuple_type = typename detail::invoke_traits<fun_type_rrt>::args_tuple_type;
		static_assert(sizeof...(_Args) == std::tuple_size<args_tuple_type>::value, "'args' count must equal argument count of 'fn'");

		using callback_type3 = detail::args_of_t<detail::get_holder_index<_Args...>::index, _Fcb>;
		using callback_type2 = std::remove_cv_t<std::remove_reference_t<callback_type3>>;
		using callback_type = typename detail::invoke_traits<callback_type2>::type;

		using first_node_type = task_cbnode<callback_type>;

		task_set_exception_agent_sptr exp = std::make_shared<task_set_exception_agent>();
		auto st_next = std::make_shared<first_node_type>(exp, std::forward<_Fcb>(fn), std::forward<_Args>(args)...);
		exp->_Impl = st_next.get();

		return task<first_node_type, first_node_type>{exp, st_next, st_next};
	}
/*
	template<class _Context, class _Fcb, class... _Args>
	inline auto marshal_task(_Context & ctx, _Fcb&& fn, _Args&&... args)
		-> decltype(ctx.add(std::declval<executor_sptr>()), make_task(detail::dummy_method_{}).marshal(ctx, std::forward<_Ftype>(fn), std::forward<_Args>(args)...))
	{
		return make_task(detail::dummy_method_{}).marshal(ctx, std::forward<_Ftype>(fn), std::forward<_Args>(args)...);
	}
*/
}

#if !LIBST_DISABLE_SHORT_NS
namespace st = lib_shark_task;
#endif