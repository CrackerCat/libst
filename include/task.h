//ʵ��һ����ʽ���õ����������ܣ�������callback hell������
//ͨ��std::future, ��ȡ���һ�����õĽ���������;�������쳣����std::future::get()���׳��쳣
//֧�ֽ���������ָ�����ض��ġ�ִ�л�������ȥ���ã��Ӷ�����ִ�е�ʱ�����߳�
//֧��C++14/17���������ʡһЩ�ڴ棬ʹ����VS��_Promiseʵ�֣���ֻ֧��VS2015/VS2017

#pragma once

#include <assert.h>
#include <future>

#include "task_utils.inl"
#include "task_invoke_traits.h"
#include "task_detail.inl"
#include "task_exception.inl"
#include "task_node.inl"
#include "task_cbnode.inl"
#include "task_executor.inl"

namespace lib_shark_task
{
	template<class _LastNode, class _FirstNode>
	struct task
	{
		using node_type = _FirstNode;
		using node_type_sptr = std::shared_ptr<node_type>;

		using last_node = _LastNode;
		using last_type = typename last_node::result_type;
	private:
		task_set_exception_agent_sptr		_Exception;			//�����쳣�Ĵ���ӿ�
		std::shared_ptr<last_node>			_Last;				//���һ������ڵ㡣ÿ��then/marshal���µ�task��������ͺ�ֵ�����仯
		mutable node_type_sptr				_Node;				//��һ������ڵ㡣����then/marshal������ڵ��ǲ��仯��
	public:
		task(const task_set_exception_agent_sptr & exp, const std::shared_ptr<last_node> & last, const std::shared_ptr<node_type> & first)
			: _Node(first)
			, _Last(last)
			, _Exception(exp)
		{
			assert(_Node != nullptr);
			assert(_Last != nullptr);
		}

		task(task && st) = default;
		task & operator = (task && st) = default;
		task(const task & st) = delete;
		task & operator = (const task & st) = delete;

		//��ʼִ���������ĵ�һ���ڵ㡣
		//ִ����Ϻ󣬽���һ������ڵ���Ϊnullptr�������Ͳ����ٴε�����
		template<class... _Args>
		void operator()(_Args&&... args) const
		{
			if (_Node == nullptr)
				std::_Throw_future_error(
					std::make_error_code(std::future_errc::no_state));

			if (!_Node->is_ready())
			{
				if (_Node->invoke_thiz(std::forward<_Args>(args)...))
					_Node->invoke_then_if();

				_Node = nullptr;
			}
		}

		//��ȡ���һ������ڵ㣬��Ӧ��future��
		auto get_future()
		{
			assert(!_Last->is_retrieved());
			return _Last->get_future();
		}

		//ִ����ֻ�ܻ�ȡһ�Ρ�
		//��ȡ��Ϻ��������ٵ���operator()��----�Ͼ���run_once����
		auto get_executor()
		{
			if (_Node == nullptr)
				std::_Throw_future_error(
					std::make_error_code(std::future_errc::no_state));

			return std::make_shared<task_executor<node_type>>(std::move(_Node));
		}

		//������һ������ڵ㣬�����µ�����������
		template<class _Ftype>
		auto then(_Ftype && fn)
		{
			assert(_Last != nullptr);
			assert(!_Last->is_retrieved());

			//DEBUG_TYPE<last_type> dt1;

			using then_result = detail::result_of_t<std::remove_reference_t<_Ftype>>;
			using next_node_type = detail::unpack_tuple_node_t<then_result, last_type>;

			auto st_next = std::make_shared<next_node_type>(fn, _Exception);
			_Exception->_Impl = st_next.get();
			_Last->_Set_then_if(detail::_Set_then_helper<next_node_type>{ st_next });

			return task<next_node_type, node_type>{_Exception, st_next, _Node};
		}

		//������һ������ڵ㣬�����µ�����������
		//ָ��������ڵ������ctx��������߳�/ʱ��������
		template<class _Context, class _Ftype>
		auto then(_Context & ctx, _Ftype && fn)
		{
			assert(_Last != nullptr);
			assert(!_Last->is_retrieved());

			using then_result = detail::result_of_t<std::remove_reference_t<_Ftype>>;
			using next_node_type = detail::unpack_tuple_node_t<then_result, last_type>;

			auto st_next = std::make_shared<next_node_type>(std::forward<_Ftype>(fn), _Exception);
			_Exception->_Impl = st_next.get();
			_Last->_Set_then_if(detail::_Set_then_ctx_helper<_Context, next_node_type>{ &ctx, st_next });

			return task<next_node_type, node_type>{_Exception, st_next, _Node};
		}

		template<class _Fcb, class... _Types>
		auto marshal(_Fcb&& fn, _Types&&... args)
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
		auto marshal(_Context & ctx, _Fcb&& fn, _Types&&... args)
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

		task_set_exception_agent_sptr & _Get_exception_agent()
		{
			return _Exception;
		}

		template<class _Nnode>
		auto _Then_node(const std::shared_ptr<_Nnode> & st_next)
		{
			assert(_Last != nullptr);
			assert(!_Last->is_retrieved());
			using next_node_type = std::remove_reference_t<_Nnode>;

			_Last->_Set_then_if(detail::_Set_then_helper<next_node_type>{ st_next });
			return task<next_node_type, node_type>{_Exception, st_next, _Node};
		}

		template<class _Context, class _Nnode>
		auto _Then_node(_Context & ctx, const std::shared_ptr<_Nnode> & st_next)
		{
			assert(_Last != nullptr);
			assert(!_Last->is_retrieved());
			using next_node_type = std::remove_reference_t<_Nnode>;

			_Last->_Set_then_if(detail::_Set_then_ctx_helper<_Context, next_node_type>{ &ctx, st_next });
			return task<next_node_type, node_type>{_Exception, st_next, _Node};
		}
	};

	template<size_t _N>
	struct _Make_task_0_impl
	{
		template<class _Ftype>
		static auto make(_Ftype && fn)
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
		static auto make(_Ftype && fn)
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

	//����һ���µ�����ڵ㣬����һ��ȫ�µ�������
	template<class _Ftype>
	auto make_task(_Ftype && fn)
	{
		using fun_type_rrt = std::remove_reference_t<_Ftype>;
		using args_tuple_type = typename detail::invoke_traits<fun_type_rrt>::args_tuple_type;

		return _Make_task_0_impl<std::tuple_size_v<args_tuple_type>>::make(std::forward<_Ftype>(fn));
	}

	//����һ���µ�����ڵ㣬����һ��ȫ�µ�������
	template<class _Ftype, class... _Args>
	auto make_task(_Ftype && fn, _Args&&... args)
	{
		using fun_type_rrt = std::remove_reference_t<_Ftype>;
		using ret_type = detail::result_of_t<fun_type_rrt>;
		using args_tuple_type = typename detail::invoke_traits<fun_type_rrt>::args_tuple_type;

		using ret_type_rrt = std::remove_reference_t<ret_type>;

		static_assert(sizeof...(_Args) == std::tuple_size_v<args_tuple_type>, "'args' count must equal argument count of 'fn'");

		using first_node_type = task_node<ret_type_rrt>;

		task_set_exception_agent_sptr exp = std::make_shared<task_set_exception_agent>();
		auto st_next = std::make_shared<first_node_type>(
			[fn = std::forward<_Ftype>(fn), args = std::make_tuple(std::forward<_Args>(args)...)]()
			{
				return std::apply(fn, args);
			}, exp);
		exp->_Impl = st_next.get();

		return task<first_node_type, first_node_type>{exp, st_next, st_next};
	}

	template<class _Fcb, class... _Args>
	auto marshal_task(_Fcb&& fn, _Args&&... args)
	{
		using fun_type_rrt = std::remove_reference_t<_Fcb>;
		using args_tuple_type = typename detail::invoke_traits<fun_type_rrt>::args_tuple_type;
		static_assert(sizeof...(_Args) == std::tuple_size_v<args_tuple_type>, "'args' count must equal argument count of 'fn'");

		using callback_type3 = detail::args_of_t<detail::get_holder_index<_Args...>::index, _Fcb>;
		using callback_type2 = std::remove_cv_t<std::remove_reference_t<callback_type3>>;
		using callback_type = typename detail::invoke_traits<callback_type2>::type;

		using first_node_type = task_cbnode<callback_type>;

		task_set_exception_agent_sptr exp = std::make_shared<task_set_exception_agent>();
		auto st_next = std::make_shared<first_node_type>(exp, std::forward<_Fcb>(fn), std::forward<_Args>(args)...);
		exp->_Impl = st_next.get();

		return task<first_node_type, first_node_type>{exp, st_next, st_next};
	}
}

#if !LIBST_DISABLE_SHORT_NS
namespace st = lib_shark_task;
#endif