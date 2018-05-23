//��һ�����е�callback��װ��������
//���͵ģ�һ��callback�����ǣ�
//	foo(..., cb)
//����
//	cb = void(args)
//Ҫ��װ��
//	tuple<args> foo2(...)
#pragma once

namespace lib_shark_task
{
	namespace detail
	{
		typedef decltype(std::placeholders::_1) placeholder_type;

		template<class... _Types>
		struct get_holder_index2;

		template<>
		struct get_holder_index2<>
		{
			static const size_t index = 0;
		};

		template<class _H1, class... _Holders>
		struct get_holder_index2<_H1, _Holders...> : public get_holder_index2<_Holders...>
		{
		};

		template<class... _Holders>
		struct get_holder_index2<placeholder_type, _Holders...>
		{
			static const size_t index = sizeof...(_Holders) + 1;
		};
		template<class... _Holders>
		struct get_holder_index2<const placeholder_type &, _Holders...>
		{
			static const size_t index = sizeof...(_Holders) + 1;
		};
		template<class... _Holders>
		struct get_holder_index2<placeholder_type &&, _Holders...>
		{
			static const size_t index = sizeof...(_Holders) + 1;
		};

		//_Holders...����������һ��������std::placeholders::_1�����_1��_Holders�ĵڼ�����
		template<class... _Holders>
		struct get_holder_index
		{
			static_assert(get_holder_index2<_Holders...>::index > 0, "get_holder_index<> have not std::placeholders::_1");

			static const size_t index = sizeof...(_Holders) - get_holder_index2<_Holders...>::index;
		};

		//static_assert(get_holder_index<placeholder_type, int, std::string>::index == 0, "place holder in 0");
		//static_assert(get_holder_index<int, placeholder_type, std::string>::index == 1, "place holder in 1");
		//static_assert(get_holder_index<int, std::string, placeholder_type>::index == 2, "place holder in 2");
		//static_assert(get_holder_index<int, placeholder_type, placeholder_type>::index == 1, "place holder in 1");
		////static_assert(get_holder_index<int, int, std::string>::index == 3, "static_assert cause failed");

		//����һ�����ݻص������ĺ������󡣽��������������ص�����
		template<class _Stype, class _Fx>
		struct callback_relay;

		template<class _Stype, class... _Types>
		struct callback_relay<_Stype, void(_Types...)>
		{
			std::shared_ptr<_Stype> _Assoc_node;

			template<class... _Types2>
			inline void operator()(_Types2&&... args)
			{
				_Assoc_node->_Do_callback(std::forward<_Types2>(args)...);
			}
		};

		template<class _Stype, class... _Types>
		struct callback_relay<_Stype, void(*)(_Types...)> : public callback_relay<_Stype, void(_Types...)>
		{
		};
		template<class _Stype, class... _Types>
		struct callback_relay<_Stype, std::function<void(_Types...)>> : public callback_relay<_Stype, void(_Types...)>
		{
		};
		template<class _Stype, class... _Types>
		struct callback_relay<_Stype, callback_relay<_Types...>> : public callback_relay<_Stype, void(_Types...)>
		{
		};
	}

	//��װ�ص�Ϊ����ڵ�
	//_Cbtype�ص����������͡��ص�������һ������(����)���������Ϊ������ڵ�ķ���ֵ
	//_PrevArgs...����һ������ڵ�ķ���ֵ(�����һ���ڵ㷵��ֵ��std::tuple<>����_PrevArgs�ǽ�tuple�Ӱ���Ĳ����б�)
	template<class _Cbtype, class... _PrevArgs>
	struct task_cbnode : public node_result_<detail::args_tuple_t<_Cbtype>>
	{
		using this_type = task_cbnode<_Cbtype, _PrevArgs...>;

		using result_type = detail::args_tuple_t<_Cbtype>;			//���ڵ�Ľ��������
		using result_tuple = result_type;							//���ڵ�Ľ�������tuple<>�������
		using args_tuple_type = std::tuple<_PrevArgs...>;			//���ڵ����δ����tuple<>�������

		template<class _Fx, class... _Types>
		task_cbnode(const task_set_exception_agent_sptr & exp, _Fx&& _Func, _Types&&... args)
			: node_result_(exp)
			, _Thiz(std::bind(std::forward<_Fx>(_Func), std::forward<_Types>(args)...))
		{
		}
		task_cbnode(task_cbnode && _Right) = default;
		task_cbnode & operator = (task_cbnode && _Right) = default;
		task_cbnode(const task_cbnode & _Right) = delete;
		task_cbnode & operator = (const task_cbnode & _Right) = delete;
	private:
		template<class... _Types2>
		void _Do_callback(_Types2&&... args)
		{
			_Set_value(std::make_tuple(std::forward<_Types2>(args)...));
			_Ready = true;
			invoke_then_if();
		}
	public:
		template<class... _PrevArgs2>
		bool invoke_thiz(_PrevArgs2&&... args)
		{
			static_assert(sizeof...(_PrevArgs2) >= typename std::tuple_size<args_tuple_type>::value, "");

			try
			{
				task_function fn = _Move_thiz();

				detail::callback_relay<this_type, _Cbtype> cb;
				cb._Assoc_node = std::static_pointer_cast<task_cbnode>(this->shared_from_this());

				detail::_Apply_then(fn, std::move(cb), std::forward<_PrevArgs2>(args)...);
				//fn(std::move(cb), std::forward<_PrevArgs2>(args)...);
			}
			catch (...)
			{
				_Set_Agent_exception(std::current_exception());
			}

			return false;
		}

		template<class _PrevTuple>
		bool invoke_thiz_tuple(_PrevTuple&& args)
		{
			static_assert(typename std::tuple_size<_PrevTuple>::value >= typename std::tuple_size<args_tuple_type>::value, "");

			try
			{
				task_function fn = _Move_thiz();

				detail::callback_relay<this_type, _Cbtype> cb;
				cb._Assoc_node = std::static_pointer_cast<task_cbnode>(this->shared_from_this());

				detail::_Apply_function<task_function>::template _Apply_cat(fn, std::move(cb), std::forward<_PrevTuple>(args));
				//std::apply(fn, std::tuple_cat(std::tuple<relay_type>{std::move(cb)}, std::forward<_PrevTuple>(args)));
			}
			catch (...)
			{
				_Set_Agent_exception(std::current_exception());
			}

			return false;
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

		template<class _Ftype>
		void _Set_then_if(_Ftype && fn)
		{
			_Set_retrieved();

			if (_Ready)
			{
				try
				{
					detail::_Apply_then2<then_function>(std::forward<_Ftype>(fn), std::move(_Get_value()));
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
				_Then = then_function{ std::forward<_Ftype>(fn) };
			}
		}

	public:
		using relay_type = detail::callback_relay<this_type, _Cbtype>;
		friend relay_type;
		using task_function = std::function<void(relay_type &&, _PrevArgs...)>;
		using then_function = detail::unpack_tuple_fn_t<void, result_type>;
	protected:
		task_function			_Thiz;			//ִ�е�ǰ����ڵ�
		then_function			_Then;			//ִ����һ������ڵ�
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

	template<class _Cbtype, class... _PrevArgs>
	struct task_cbnode<_Cbtype, std::tuple<_PrevArgs...>> : public task_cbnode<_Cbtype, _PrevArgs...>
	{
		using task_cbnode<_Cbtype, _PrevArgs...>::task_cbnode;
	};
}
