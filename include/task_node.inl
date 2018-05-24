
#pragma once

#ifdef _WIN32
	#include "task_node_win32.inl"
#elif __APPLE__ 
	#include "TargetConditionals.h" 
	#if TARGET_IPHONE_SIMULATOR // iOS Simulator 
		#include "task_node_ios.inl"
	#elif TARGET_OS_IPHONE // iOS device 
		#include "task_node_ios.inl"
	#elif TARGET_OS_MAC // Other kinds of Mac OS 
		#include "task_node_ios.inl"
	#else 
		#error "Unknown Apple platform" 
	#endif 
#elif __ANDROID__ // android 
	#include "task_node_android.inl"
#elif __linux__ // linux 
	#include "task_node_linux.inl"
#elif __unix__ // all unices not caught above // Unix 
	#include "task_node_unix.inl"
#elif defined(_POSIX_VERSION) // POSIX 
	#include "task_node_posix.inl"
#else 
	#error "Unknown platform" 
#endif 

namespace lib_shark_task
{
	//��task_state_result�����ϣ��ṩ����ִ�е�ǰ����ڵ㣬ִ����һ������ڵ�ĺ���
	template<class _Rtype, class _Taskf, class _Thenf = std::function<void()>>
	struct node_impl : public node_result_<_Rtype>
	{
		using task_function = std::remove_reference_t<_Taskf>;
		using then_function = std::remove_reference_t<_Thenf>;
	protected:
		task_function			_Thiz;			//ִ�е�ǰ����ڵ�
		then_function			_Then;			//ִ����һ������ڵ�
	public:
		node_impl(task_function && fn, const task_set_exception_agent_sptr & exp)
			: node_result_<_Rtype>(exp)
			, _Thiz(std::forward<task_function>(fn))
		{
		}
		node_impl(const task_function & fn, const task_set_exception_agent_sptr & exp)
			: node_result_<_Rtype>(exp)
			, _Thiz(fn)
		{
		}
		node_impl(const task_set_exception_agent_sptr & exp)
			: node_result_<_Rtype>(exp)
		{
		}

		node_impl(node_impl && _Right) = default;
		node_impl & operator = (node_impl && _Right) = default;
		node_impl(const node_impl & _Right) = delete;
		node_impl & operator = (const node_impl & _Right) = delete;
	protected:
		//ȡִ�е�ǰ����ڵ�ĺ�����ֻ��ȡһ�Ρ��̰߳�ȫ
		inline task_function _Move_thiz()
		{
			std::unique_lock<std::mutex> _Lock(this->_Mtx());
			return std::move(_Thiz);			//ǿ��ֻ�ܵ���һ��
		}
		//ȡִ����һ������ڵ�ĺ�����ֻ��ȡһ�Ρ��̰߳�ȫ
		inline then_function _Move_then()
		{
			std::unique_lock<std::mutex> _Lock(this->_Mtx());
			return std::move(_Then);			//ǿ��ֻ�ܵ���һ��
		}
	};

	//����ڵ�
	//_Rtype�Ǳ��ڵ�ķ���ֵ����
	//_PrevArgs...����һ������ڵ�ķ���ֵ(�����һ���ڵ㷵��ֵ��std::tuple<>����_PrevArgs�ǽ�tuple�Ӱ���Ĳ����б�)
	template<class _Rtype, class... _PrevArgs>
	struct task_node : public node_impl<std::remove_reference_t<_Rtype>,
						std::function<std::remove_reference_t<_Rtype>(_PrevArgs...)>, 
						detail::unpack_tuple_fn_t<void, std::remove_reference_t<_Rtype>> >
	{
		using result_type = std::remove_reference_t<_Rtype>;			//���ڵ�Ľ��������
		using result_tuple = detail::package_tuple_t<result_type>;		//���ڵ�Ľ�������tuple<>�������
		using args_tuple_type = std::tuple<_PrevArgs...>;				//���ڵ����δ����tuple<>�������

		using base_type = node_impl<std::remove_reference_t<_Rtype>,
			std::function<std::remove_reference_t<_Rtype>(_PrevArgs...)>,
			detail::unpack_tuple_fn_t<void, std::remove_reference_t<_Rtype>> >;
		using task_function = typename base_type::task_function;
		using then_function = typename base_type::then_function;
		using base_type::base_type;

		template<class... _PrevArgs2>
		bool invoke_thiz(_PrevArgs2&&... args)
		{
			static_assert(sizeof...(_PrevArgs2) >= std::tuple_size<args_tuple_type>::value, "");

			try
			{
				task_function fn = this->_Move_thiz();
				this->_Set_value(detail::_Apply_then(fn, std::forward<_PrevArgs2>(args)...));
				//_Set_value(fn(std::forward<_PrevArgs2>(args)...));
				this->_Ready = true;
			}
			catch (...)
			{
				this->_Set_Agent_exception(std::current_exception());
			}

			return this->_Ready;
		}

		template<class _PrevTuple>
		bool invoke_thiz_tuple(_PrevTuple&& args)
		{
			static_assert(std::tuple_size<_PrevTuple>::value >= std::tuple_size<args_tuple_type>::value, "");

			try
			{
				task_function fn = this->_Move_thiz();
				this->_Set_value(detail::_Apply_then(fn, std::forward<_PrevTuple>(args)));
				//_Set_value(std::apply(fn, std::forward<_PrevTuple>(args)));
				this->_Ready = true;
			}
			catch (...)
			{
				this->_Set_Agent_exception(std::current_exception());
			}

			return this->_Ready;
		}

		void invoke_then_if()
		{
			if (!this->_Ready)
				return;

			then_function fn = this->_Move_then();
			if (!fn)
				return;

			try
			{
				detail::_Apply_then(fn, std::move(this->_Get_value()));
				//detail::_Invoke_then(fn, std::move(_Get_value()));
			}
			catch (...)
			{
				this->_Set_Agent_exception(std::current_exception());
			}
		}

		template<class _NextFx>
		void _Set_then_if(_NextFx && fn)
		{
			this->_Set_retrieved();

			if (this->_Ready)
			{
				try
				{
					detail::_Apply_then2<then_function>(std::forward<_NextFx>(fn), std::move(this->_Get_value()));
					//detail::_Invoke_then(fn, std::move(_Get_value()));
				}
				catch (...)
				{
					this->_Set_Agent_exception(std::current_exception());
				}
			}
			else
			{
				std::unique_lock<std::mutex> _Lock(this->_Mtx());
				this->_Then = then_function{ std::forward<_NextFx>(fn) };
			}
		}
	};

	template<class... _PrevArgs>
	struct task_node<void, _PrevArgs...> : public node_impl<int, std::function<void(_PrevArgs...)>>
	{
		using result_type = void;									//���ڵ�Ľ��������
		using result_tuple = std::tuple<>;							//���ڵ�Ľ�������tuple<>�������
		using args_tuple_type = std::tuple<_PrevArgs...>;			//���ڵ����δ����tuple<>�������

		using base_type = node_impl<int, std::function<void(_PrevArgs...)>>;
		using task_function = typename base_type::task_function;
		using then_function = typename base_type::then_function;
		using base_type::base_type;

		template<class... _PrevArgs2>
		bool invoke_thiz(_PrevArgs2&&... args)
		{
			static_assert(sizeof...(_PrevArgs2) >= std::tuple_size<args_tuple_type>::value, "");

			try
			{
				task_function fn = this->_Move_thiz();
				detail::_Apply_then(fn, std::forward<_PrevArgs2>(args)...);
				//fn(std::forward<_PrevArgs2>(args)...);
				this->_Set_value(0);
				this->_Ready = true;
			}
			catch (...)
			{
				this->_Set_Agent_exception(std::current_exception());
			}

			return this->_Ready;
		}

		template<class _PrevTuple>
		bool invoke_thiz_tuple(_PrevTuple&& args)
		{
			static_assert(std::tuple_size<_PrevTuple>::value >= std::tuple_size<args_tuple_type>::value, "");

			try
			{
				task_function fn = this->_Move_thiz();
				detail::_Apply_then(fn, std::forward<_PrevTuple>(args));
				//std::apply(fn, std::forward<_PrevTuple>(args));
				this->_Set_value(0);
				this->_Ready = true;
			}
			catch (...)
			{
				this->_Set_Agent_exception(std::current_exception());
			}
			return this->_Ready;
		}

		void invoke_then_if()
		{
			if (!this->_Ready)
				return;

			then_function fn = this->_Move_then();
			if (!fn)
				return;

			try
			{
				fn();
			}
			catch (...)
			{
				this->_Set_Agent_exception(std::current_exception());
			}
		}

		template<class _NextFx>
		void _Set_then_if(_NextFx && fn)
		{
			this->_Set_retrieved();

			if (this->_Ready)
			{
				try
				{
					fn();
				}
				catch (...)
				{
					this->_Set_Agent_exception(std::current_exception());
				}
			}
			else
			{
				std::unique_lock<std::mutex> _Lock(this->_Mtx());
				this->_Then = std::forward<_NextFx>(fn);
			}
		}
	};

	template<class _Rtype, class... _PrevArgs>
	struct task_node<_Rtype, std::tuple<_PrevArgs...>> : public task_node<_Rtype, _PrevArgs...>
	{
		using task_node<_Rtype, _PrevArgs...>::task_node;
	};
}
