
#pragma once

namespace lib_shark_task
{
#ifdef ANDROID
	template<class _Rtype>
	struct __assoc_state_hack : public std::__assoc_state<_Rtype>
	{
		std::mutex & _Mtx()
		{
			return this->__mut_;
		}
		_Rtype _Get_value()
		{
			std::unique_lock<std::mutex> __lk(this->__mut_);

			this->__state_ |= std::__assoc_state<_Rtype>::__future_attached;
			return this->__value_;
		}
		_Rtype & _Peek_value()
		{
			return this->__value_;
		}
	};

	//ʵ�ִ�ȡ����ڵ�Ľ����
	//��ȡ���ֵ
	//���ض�Ӧ��future����
	//�ṩmutexʵ��
	template<class _Rtype>
	struct node_result_ : public std::enable_shared_from_this<node_result_<_Rtype>>
							, public task_set_exception
	{
	protected:
		__assoc_state_hack<_Rtype>*		_State;						//ͨ��clang��future�ڲ���__assoc_state��ʵ��future���ܡ�������Լ���future
		task_set_exception_agent_sptr	_Exception;					//�����쳣�Ĵ���ӿ�
		std::atomic<bool>				_Ready{ false };			//����Ƿ��Ѿ�׼�����ˣ��̰߳�ȫ

	public:
		node_result_(const task_set_exception_agent_sptr & exp)
			: _State(new __assoc_state_hack<_Rtype>)
			, _Exception(exp)
		{
		}
		~node_result_()
		{
			if (_State)
				_State->__release_shared();
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
			return std::__make_deferred_assoc_state<_Rtype>([this]()
			{
				return _State;
			});
			//return (std::future<_Rtype>(_State));
		}

		//����Ƿ��Ѿ�׼�����ˣ��̰߳�ȫ
		inline bool is_ready() const
		{
			return _Ready;
		}

		//�Ƿ��Ѿ����ù�get_future/_Set_then_if/_Get_value֮һ
		inline bool is_retrieved() const
		{
			return _State->__has_future_attached();
		}
	private:
		//task_set_exception �ӿڵ�ʵ��
		virtual void _Set_exception(std::exception_ptr && val) override
		{
			_State->set_exception(std::forward<std::exception_ptr>(val));
		}
	protected:
		inline std::__assoc_state<_Rtype> * _Ptr() const
		{
			return _State;
		}
		//�趨���ù�get_future/_Set_then_if/_Get_value֮һ
		void _Set_retrieved()
		{
			if (_State->__has_future_attached())
				throw std::future_error(std::make_error_code(std::future_errc::no_state));
			_State->__set_future_attached();
		}
		//��ȡ���ֵ��ֻ�ܵ���һ��
		inline _Rtype _Get_value()
		{
			return _State->_Get_value();
		}
		inline _Rtype & _Peek_value()
		{
			return _State->_Peek_value();
		}
		//���ô��ֵ
		template<class _Ty2>
		inline void _Set_value(_Ty2 && val)
		{
			_State->set_value(std::forward<_Ty2>(val));
		}
		//�����쳣
		inline void _Set_Agent_exception(std::exception_ptr && val)
		{
			_Exception->_Set_exception(std::forward<std::exception_ptr>(val));
		}
		//�����ṩ��mutexʵ��
		inline std::mutex & _Mtx() const
		{
			return _State->_Mtx();
		}
	};

#endif
}
