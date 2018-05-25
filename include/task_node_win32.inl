
#pragma once

namespace lib_shark_task
{
#if defined(_MSC_VER)
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
		inline void _Set_value()
		{
			_Ptr()->_Set_value(false);
		}
		//�����쳣
		inline void _Set_Agent_exception(std::exception_ptr && val)
		{
			_Exception->_Set_exception(std::forward<std::exception_ptr>(val));
		}
		//�����ṩ��mutexʵ��
		inline std::mutex & _Mtx() const
		{
			return _Ptr()->_Mtx;
		}
	};
#else
	#error "Unknown Compiler on Windows" 
#endif
}
