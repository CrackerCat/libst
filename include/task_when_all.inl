#pragma once
#include "task_when_node.inl"

namespace lib_shark_task
{
	//when_all(_Task1, _Task2, ...)
	//�����ڵȴ�һ�鲻ͬ���͵�����ȫ����ɡ�
	//���ڷ��ؽ�����Ͳ�ͬ������Щ����ķ���ֵ�������Ⱥ�˳�򣬴����һ��std::tuple<result_type...>��
	//
	//_All_tasks ������������Ͳ�ͬ������ֻ����һ��std::tuple<>������
	//��ˣ�invoke_thiz/invoke_thiz_tuple����for_each(std::tuple<>)�﷨������_All_tasks
	//
	//���ڷ��ؽ����ͬ
	//_Set_value_partial/_Set_value_partial_t �����������_Idxָʾ����std::tuple<result_type...>��Ӧλ���ϡ�
	//
	//_On_result ������н���Ѿ���ã������invoke_then_if
	template<class _Ttuple, class... _ResultArgs>
	struct task_all_node : public node_impl<std::tuple<_ResultArgs...>, std::function<void()>, std::function<void(_ResultArgs...)>>
	{
		using base_type = node_impl<std::tuple<_ResultArgs...>, std::function<void()>, std::function<void(_ResultArgs...)>>;
		using this_type = task_all_node<_Ttuple, _ResultArgs...>;

		using element_type = std::tuple<_ResultArgs...>;	//�ȴ��Ľڵ�Ľ������
		using result_type = std::tuple<_ResultArgs...>;		//���ڵ�Ľ��������
		using result_tuple = result_type;					//���ڵ�Ľ�������tuple<>�������
		using args_tuple_type = std::tuple<>;				//���ڵ����δ����tuple<>�������

		using task_tuple = detail::package_tuple_t<_Ttuple>;
		task_tuple			_All_tasks;

		template<class... _Tasks>
		task_all_node(const task_set_exception_agent_sptr & exp, _Tasks&&... ts)
			: base_type(exp)
			, _All_tasks(std::forward<_Tasks>(ts)...)
			, _Result_count(std::tuple_size<_Ttuple>::value)
		{
		}
		task_all_node(task_all_node && _Right) = default;
		task_all_node & operator = (task_all_node && _Right) = default;
		task_all_node(const task_all_node & _Right) = delete;
		task_all_node & operator = (const task_all_node & _Right) = delete;

	private:
		std::atomic<intptr_t> _Result_count;
	public:
		template<size_t _Idx, class... _PrevArgs2>
		void _Set_value_partial(size_t, _PrevArgs2&&... args)
		{
			std::unique_lock<std::mutex> _Lock(this->_Mtx());
			detail::_Fill_to_tuple<_Idx>(this->_Peek_value(), std::forward<_PrevArgs2>(args)...);
		}
		template<size_t _Idx, class _PrevTuple>
		void _Set_value_partial_t(size_t, _PrevTuple&& args)
		{
			std::unique_lock<std::mutex> _Lock(this->_Mtx());
			detail::_Move_to_tuple<_Idx>(this->_Peek_value(), std::forward<_PrevTuple>(args));
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
				task_tuple all_task = std::move(_All_tasks);
				_Lock.unlock();

				for_each(all_task, [&](auto & ts)
				{
					ts(args...);
				});
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
				task_tuple all_task = std::move(_All_tasks);
				_Lock.unlock();

				for_each(all_task, [&](auto & ts)
				{
					std::apply(ts, args);
				});
			}
			catch (...)
			{
				this->_Set_Agent_exception(std::current_exception());
			}

			return false;
		}
	};
	template<class _Ttuple, class... _ResultArgs>
	struct task_all_node<_Ttuple, std::tuple<_ResultArgs...>> : public task_all_node<_Ttuple, _ResultArgs...>
	{
		using task_all_node<_Ttuple, _ResultArgs...>::task_all_node;
	};
}
