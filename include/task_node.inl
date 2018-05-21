
#pragma once

namespace lib_shark_task
{
	//ʵ�ִ�ȡ����ڵ�Ľ����
	//��ȡ���ֵ
	//���ض�Ӧ��future����
	//�ṩmutexʵ��
	template<class _Rtype>
	struct node_result_ : public std::enable_shared_from_this<node_result_<_Rtype>>
							, public task_set_exception
	{
	protected:
		std::_State_manager<_Rtype>		_State;						//ͨ��VS��future�ڲ���_State_manager��ʵ��future���ܡ�������Լ���future
		task_set_exception_agent_sptr	_Exception;					//�����쳣�Ĵ���ӿ�
		std::atomic<bool>				_Ready = false;				//����Ƿ��Ѿ�׼�����ˣ��̰߳�ȫ
		bool							_Future_retrieved = false;	//�Ƿ��Ѿ����ù�get_future()�������Ѿ����ù�_Set_then_if

	public:
		node_result_(const task_set_exception_agent_sptr & exp)
			: _State(new std::_Associated_state<_Rtype>, true)
			, _Exception(exp)
		{
		}
		node_result_(node_result_ && _Right) = default;
		node_result_ & operator = (node_result_ && _Right) = default;
		node_result_(const node_result_ & _Right) = delete;
		node_result_ & operator = (const node_result_ & _Right) = delete;
		
		//�˺���ֻӦ�ñ�����һ�Ρ�
		//�����ⲿû�е��ù� _Set_then_if()
		//�ڲ����ܵ��ù�_Get_value()
		inline std::future<_Rtype> get_future()
		{
			assert(!is_retrieved());

			_Set_retrieved();
			return (std::future<_Rtype>(_State, std::_Nil()));
		}

		//����Ƿ��Ѿ�׼�����ˣ��̰߳�ȫ
		inline bool is_ready() const
		{
			return _Ready;
		}

		//�Ƿ��Ѿ����ù�get_future/_Set_then_if/_Get_value֮һ
		inline bool is_retrieved() const
		{
			std::unique_lock<std::mutex> _Lock(_Mtx());

			return _Ptr()->_Already_retrieved() || _Future_retrieved;
		}
	private:
		//task_set_exception �ӿڵ�ʵ��
		virtual void _Set_exception(std::exception_ptr && val) override
		{
			_Ptr()->_Set_exception(std::forward<std::exception_ptr>(val), false);
		}
	protected:
		inline std::_Associated_state<_Rtype> * _Ptr() const
		{
			return _State._Ptr();
		}
		//�趨���ù�get_future/_Set_then_if/_Get_value֮һ
		void _Set_retrieved()
		{
			std::unique_lock<std::mutex> _Lock(_Mtx());

			if (!_State.valid())
				std::_Throw_future_error(
					std::make_error_code(std::future_errc::no_state));
			if (_Future_retrieved)
				std::_Throw_future_error(
					std::make_error_code(std::future_errc::future_already_retrieved));
			_Future_retrieved = true;
		}
		//��ȡ���ֵ��ֻ�ܵ���һ��
		inline _Rtype && _Get_value()
		{
			return std::move(_Ptr()->_Get_value(true));
		}
		inline _Rtype & _Peek_value()
		{
			return _Ptr()->_Result;
		}
		//���ô��ֵ
		template<class _Ty2>
		inline void _Set_value(_Ty2 && val)
		{
			_Ptr()->_Set_value(std::forward<_Ty2>(val), false);
		}
		//�����쳣
		inline void _Set_Agent_exception(std::exception_ptr && val)
		{
			_Exception->_Set_exception(std::current_exception());
		}
		//�����ṩ��mutexʵ��
		inline std::mutex & _Mtx() const
		{
			return _Ptr()->_Mtx;
		}
	};

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
			: node_result_(exp)
			, _Thiz(std::forward<task_function>(fn))
		{
		}
		node_impl(const task_function & fn, const task_set_exception_agent_sptr & exp)
			: node_result_(exp)
			: _Thiz(fn)
		{
		}
		node_impl(const task_set_exception_agent_sptr & exp)
			: node_result_(exp)
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
			std::unique_lock<std::mutex> _Lock(_Mtx());
			return std::move(_Thiz);			//ǿ��ֻ�ܵ���һ��
		}
		//ȡִ����һ������ڵ�ĺ�����ֻ��ȡһ�Ρ��̰߳�ȫ
		inline then_function _Move_then()
		{
			std::unique_lock<std::mutex> _Lock(_Mtx());
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

		using node_impl::node_impl;

		template<class... _PrevArgs2>
		bool invoke_thiz(_PrevArgs2&&... args)
		{
			static_assert(sizeof...(_PrevArgs2) >= typename std::tuple_size<args_tuple_type>::value, "");

			try
			{
				task_function fn = _Move_thiz();
				_Set_value(detail::_Apply_then(fn, std::forward<_PrevArgs2>(args)...));
				//_Set_value(fn(std::forward<_PrevArgs2>(args)...));
				_Ready = true;
			}
			catch (...)
			{
				_Set_Agent_exception(std::current_exception());
			}

			return _Ready;
		}

		template<class _PrevTuple>
		bool invoke_thiz_tuple(_PrevTuple&& args)
		{
			static_assert(typename std::tuple_size<_PrevTuple>::value >= typename std::tuple_size<args_tuple_type>::value, "");

			try
			{
				task_function fn = _Move_thiz();
				_Set_value(detail::_Apply_then(fn, std::forward<_PrevTuple>(args)));
				//_Set_value(std::apply(fn, std::forward<_PrevTuple>(args)));
				_Ready = true;
			}
			catch (...)
			{
				_Set_Agent_exception(std::current_exception());
			}

			return _Ready;
		}

		void invoke_then_if()
		{
			if (!_Ready)
				return;

			then_function fn = _Move_then();
			if (!fn)
				return;

			try
			{
				detail::_Apply_then(fn, std::move(_Get_value()));
				//detail::_Invoke_then(fn, std::move(_Get_value()));
			}
			catch (...)
			{
				_Set_Agent_exception(std::current_exception());
			}
		}

		template<class _NextFx>
		void _Set_then_if(_NextFx && fn)
		{
			_Set_retrieved();

			if (_Ready)
			{
				try
				{
					detail::_Apply_then2<then_function>(std::forward<_NextFx>(fn), std::move(_Get_value()));
					//detail::_Invoke_then(fn, std::move(_Get_value()));
				}
				catch (...)
				{
					_Set_Agent_exception(std::current_exception());
				}
			}
			else
			{
				std::unique_lock<std::mutex> _Lock(_Mtx());
				_Then = then_function{ std::forward<_NextFx>(fn) };
			}
		}
	};

	template<class... _PrevArgs>
	struct task_node<void, _PrevArgs...> : public node_impl<int, std::function<void(_PrevArgs...)>>
	{
		using result_type = void;									//���ڵ�Ľ��������
		using result_tuple = std::tuple<>;							//���ڵ�Ľ�������tuple<>�������
		using args_tuple_type = std::tuple<_PrevArgs...>;			//���ڵ����δ����tuple<>�������

		using node_impl::node_impl;

		template<class... _PrevArgs2>
		bool invoke_thiz(_PrevArgs2&&... args)
		{
			static_assert(sizeof...(_PrevArgs2) >= typename std::tuple_size<args_tuple_type>::value, "");

			try
			{
				task_function fn = _Move_thiz();
				detail::_Apply_then(fn, std::forward<_PrevArgs2>(args)...);
				//fn(std::forward<_PrevArgs2>(args)...);
				_Set_value(0);
				_Ready = true;
			}
			catch (...)
			{
				_Set_Agent_exception(std::current_exception());
			}

			return _Ready;
		}

		template<class _PrevTuple>
		bool invoke_thiz_tuple(_PrevTuple&& args)
		{
			static_assert(typename std::tuple_size<_PrevTuple>::value >= typename std::tuple_size<args_tuple_type>::value, "");

			try
			{
				task_function fn = _Move_thiz();
				detail::_Apply_then(fn, std::forward<_PrevTuple>(args));
				//std::apply(fn, std::forward<_PrevTuple>(args));
				_Set_value(0);
				_Ready = true;
			}
			catch (...)
			{
				_Set_Agent_exception(std::current_exception());
			}
			return _Ready;
		}

		void invoke_then_if()
		{
			if (!_Ready)
				return;

			then_function fn = _Move_then();
			if (!fn)
				return;

			try
			{
				fn();
			}
			catch (...)
			{
				_Set_Agent_exception(std::current_exception());
			}
		}

		template<class _NextFx>
		void _Set_then_if(_NextFx && fn)
		{
			_Set_retrieved();

			if (_Ready)
			{
				try
				{
					fn();
				}
				catch (...)
				{
					_Set_Agent_exception(std::current_exception());
				}
			}
			else
			{
				std::unique_lock<std::mutex> _Lock(_Mtx());
				_Then = std::forward<_NextFx>(fn);
			}
		}
	};

	template<class _Rtype, class... _PrevArgs>
	struct task_node<_Rtype, std::tuple<_PrevArgs...>> : public task_node<_Rtype, _PrevArgs...>
	{
		using task_node<_Rtype, _PrevArgs...>::task_node;
	};
}
