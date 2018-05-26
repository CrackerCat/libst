#pragma once
#include <vector>
#include "task_when_node.inl"

namespace lib_shark_task
{
	//when_all(_Iter begin, _Iter end)
	//������ͨ�������ӵȴ�һ������ȫ����ɡ�
	//��ˣ����е�����ķ���ֵ��Ȼ��ͬ�������std::vector<result_type>��
	//
	//_All_tasks ���������������ͬ������ʹ��һ��std::vector<>�����档
	//��ˣ�invoke_thiz/invoke_thiz_tuple����ranged for(std::vector<>)�﷨������_All_tasks
	//
	//���ڷ��ؽ����ͬ
	//_Set_value_partial/_Set_value_partial_t �����������idxָʾ����std::vector<result_type>��Ӧλ���ϡ�
	//
	//_On_result ������н���Ѿ���ã������invoke_then_if
	template<class _Ttype, class _ResultArgs>
	struct task_allv_node : public node_impl<std::vector<_ResultArgs>, std::function<void()>, std::function<void(std::vector<_ResultArgs>)>>
	{
		using base_type = node_impl<std::vector<_ResultArgs>, std::function<void()>, std::function<void(std::vector<_ResultArgs>)>>;
		using this_type = task_allv_node<_Ttype, _ResultArgs>;

		using element_type = _ResultArgs;
		using result_type = std::vector<_ResultArgs>;				//���ڵ�Ľ��������
		using result_tuple = std::tuple<result_type>;				//���ڵ�Ľ�������tuple<>�������
		using args_tuple_type = std::tuple<>;						//���ڵ����δ����tuple<>�������

		using task_vector = std::vector<_Ttype>;
		task_vector			_All_tasks;

		template<class _Iter>
		task_allv_node(const task_set_exception_agent_sptr & exp, _Iter _First, _Iter _Last)
			: base_type(exp)
		{
			_All_tasks.reserve(std::distance(_First, _Last));
			for (; _First != _Last; ++_First)
				_All_tasks.emplace_back(std::move(*_First));
			
			_Result_count = _All_tasks.size();
			this->_Peek_value().resize(_Result_count);
		}
		task_allv_node(task_allv_node && _Right) = default;
		task_allv_node & operator = (task_allv_node && _Right) = default;
		task_allv_node(const task_allv_node & _Right) = delete;
		task_allv_node & operator = (const task_allv_node & _Right) = delete;

	private:
		std::atomic<intptr_t> _Result_count;
	public:
		template<size_t _Idx, class... _PrevArgs2>
		void _Set_value_partial(size_t idx, _PrevArgs2&&... args)
		{
			assert(idx < this->_Peek_value().size());

			std::unique_lock<std::mutex> _Lock(this->_Mtx());
			detail::_Fill_to_tuple<_Idx>(this->_Peek_value()[idx], std::forward<_PrevArgs2>(args)...);
		}
		template<size_t _Idx, class _PrevTuple>
		void _Set_value_partial_t(size_t idx, _PrevTuple&& args)
		{
			assert(idx < this->_Peek_value().size());

			std::unique_lock<std::mutex> _Lock(this->_Mtx());
			this->_Peek_value()[idx] = std::forward<_PrevTuple>(args);
		}

		void _On_result(size_t)
		{
			if (--_Result_count == 0)
			{
				this->_Set_value();
				this->_Ready = true;
				this->invoke_then_if();
			}
		}

		template<class... Args2>
		bool invoke_thiz(Args2&&... args)
		{
			static_assert(sizeof...(Args2) >= std::tuple_size<args_tuple_type>::value, "");

			try
			{
				std::unique_lock<std::mutex> _Lock(this->_Mtx());
				task_vector all_task = std::move(_All_tasks);
				_Lock.unlock();

				if (all_task.size() > 0)
				{
					for (auto & ts : all_task)
						ts(args...);
				}
				else
				{
					this->_Set_value();
					this->_Ready = true;
					return true;
				}
			}
			catch (...)
			{
				this->_Set_Agent_exception(std::current_exception());
			}

			return false;
		}

		template<class _PrevTuple>
		bool invoke_thiz_tuple(_PrevTuple&& args)
		{
			static_assert(std::tuple_size<_PrevTuple>::value >= std::tuple_size<args_tuple_type>::value, "");

			try
			{
				std::unique_lock<std::mutex> _Lock(this->_Mtx());
				task_vector all_task = std::move(_All_tasks);
				_Lock.unlock();

				if (all_task.size() > 0)
				{
					for (auto & ts : all_task)
						std::apply(ts, args);
				}
				else
				{
					this->_Set_value();
					this->_Ready = true;
					return true;
				}
			}
			catch (...)
			{
				this->_Set_Agent_exception(std::current_exception());
			}

			return false;
		}
	};
	template<class _Ttuple, class... _ResultArgs>
	struct task_allv_node<_Ttuple, std::tuple<_ResultArgs...>> : public task_allv_node<_Ttuple, _ResultArgs...>
	{
		using task_allv_node<_Ttuple, _ResultArgs...>::task_allv_node;
	};

}
