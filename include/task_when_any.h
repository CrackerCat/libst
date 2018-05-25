#pragma once
#include "task_when_node.inl"
//#include "task_when_any.inl"
#include "task_when_anyv.inl"

namespace lib_shark_task
{
	namespace detail
	{
		template<size_t _Idx, class _Anode>
		void when_any_impl2(_Anode * all_node, size_t node_idx)
		{
		}

		template<size_t _Idx, class _Anode, class _Task>
		auto when_any_one_impl(_Anode * all_node, size_t node_idx, _Task & tf)
		{
			using tuple_type = decltype(declval_task_last_node_result_tuple<_Task>());

			using task_type = std::remove_reference_t<_Task>;
			using result_tuple = typename _Anode::result_tuple;
			using node_args_type = when_node_args<_Anode, result_tuple, _Idx>;

			using next_node_type = task_when_one<node_args_type, tuple_type>;

			task_set_exception_agent_sptr exp = tf._Get_exception_agent();
			exp->_Impl = all_node;

			auto st_next = std::make_shared<next_node_type>(exp, all_node, node_idx);
			return tf.template _Then_node<next_node_type>(st_next);
		}

		template<size_t _Idx, class _Anode, class _Task, class... _TaskRest>
		void when_any_impl2(_Anode * all_node, size_t node_idx, _Task && tf, _TaskRest&&... rest)
		{
			when_any_one_impl<_Idx>(all_node, node_idx, tf);

			using tuple_type = decltype(declval_task_last_node_result_tuple<_Task>());
			when_any_impl2<_Idx + std::tuple_size<tuple_type>::value>(all_node, ++node_idx, std::forward<_TaskRest>(rest)...);
		}

		template<class _Anode, class _Ttuple, size_t... Idx>
		void when_any_impl(_Anode * all_node, _Ttuple & tasks, std::index_sequence<Idx...>)
		{
			when_any_impl2<0>(all_node, 0, std::get<Idx>(tasks)...);
		}
	}

	//�ȴ�����������
	//�������Ľ��������һ��ƴ�ϵ�tuple<>��
	//������һ��ȫ�µ�task<task_all_node, task_all_node>
	//	task_all_node::invoke_thiz ��Ҫ����������е������Ա��ڿ�ʼ����
	//		Ϊÿ��������һ��task_when_one��
	//		task_when_one ���𽫽�����뵽 task_all_node��ƴ��tuple<>�Ȼ��֪ͨ task_all_node ��һ���������
	//		task_all_node ������������ɺ󣬵���invoke_then_if
	//
/*
	template<class _Task, class... _TaskRest>
	auto when_all(_Task& tfirst, _TaskRest&... rest)
	{
		static_assert(detail::is_task<_Task>::value, "use 'make_task' or 'marshal_task' to create a task");
		(void)std::initializer_list<int>{detail::check_task_type<_TaskRest>()...};

		using cated_task_t = std::tuple<std::remove_reference_t<_Task>, std::remove_reference_t<_TaskRest>...>;
		using cated_result_t = decltype(std::tuple_cat(detail::declval_task_last_node_result_tuple<_Task>(), detail::declval_task_last_node_result_tuple<_TaskRest>()...));

		task_set_exception_agent_sptr exp = tfirst._Get_exception_agent();

		using first_node_type = task_all_node<cated_task_t, cated_result_t>;
		auto st_first = std::make_shared<first_node_type>(exp, std::move(tfirst), std::move(rest)...);
		exp->_Impl = st_first.get();

		detail::when_all_impl(st_first.get(), st_first->_All_tasks, std::make_index_sequence<std::tuple_size<cated_task_t>::value>{});

		return task<first_node_type, first_node_type>{exp, st_first, st_first};
	}
*/

	//�ȴ��������֮һ���
	//�������Ľ�����Ϳ϶���һ�µģ��ʷ���ֵ�ĵ�һ������ָʾ����һ��������ɣ����������ǽ��
	//������һ��ȫ�µ�task<task_anyv_node, task_anyv_node>
	//	task_anyv_node::invoke_thiz ��Ҫ����������е������Ա��ڿ�ʼ����������Ϊ��ʱ������ȫ�������Ѿ���ɴ���
	//		Ϊÿ��������һ��task_when_one��
	//		task_when_one ���𽫽�����뵽 task_anyv_node��tuple<size_t, Result...>�Ȼ��֪ͨ task_anyv_node ��һ���������
	//		task_anyv_node ������������ɺ󣬵���invoke_then_if
	//
	template<class _Iter, typename _Fty = std::enable_if_t<detail::is_task<decltype(*std::declval<_Iter>())>::value, decltype(*std::declval<_Iter>())>>
	auto when_any(_Iter _First, _Iter _Last)
	{
		using task_type = typename std::remove_reference<_Fty>::type;
		static_assert(detail::is_task<task_type>::value, "_Iter must be a iterator of task container");
		using result_type = decltype(detail::declval_task_last_node_result_tuple<task_type>());

		return detail::when_iter<task_anyv_node<task_type, result_type> >(_First, _Last);
	}
}
