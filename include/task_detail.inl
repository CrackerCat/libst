#pragma once

namespace lib_shark_task
{
	template<class _Rtype, class... _Args>
	struct task_node;
	template<class _Cbtype, class... _Args>
	struct task_cbnode;

	namespace detail
	{
		template<class _Fx>
		using result_of_t = typename invoke_traits<_Fx>::result_type;			//��ȡ����(����)_Fx�ķ���ֵ
		template<class _Fx>
		using args_tuple_t = typename invoke_traits<_Fx>::args_tuple_type;		//��ȡ����(����)_Fx�Ĳ����б���������б���std::tuple<>�����
		template<size_t _Idx, class _Fx>
		using args_of_t = typename invoke_traits<_Fx>::template args_element<_Idx>::type;	//��ȡ����(����)_Fx���ض������Ĳ�������

		template<size_t _Idx, class _Tuple>
		void _Fill_to_tuple_impl(_Tuple & t) {}
		template<size_t _Idx, class _Tuple, class _Ty, class... _Rest>
		void _Fill_to_tuple_impl(_Tuple & t, _Ty && val, _Rest&&... args)
		{
			std::get<_Idx>(t) = std::forward<_Ty>(val);
			_Fill_to_tuple_impl<_Idx + 1>(t, std::forward<_Rest>(args)...);
		}

		template<size_t _Idx, class _Tuple, class... _Args>
		void _Fill_to_tuple(_Tuple & t, _Args&&... args)
		{
			_Fill_to_tuple_impl<_Idx>(t, std::forward<_Args>(args)...);
		}
		template<size_t _Idx, class _Tuple, class... _Args>
		void _Fill_to_tuple(_Tuple & t, const _Args&... args)
		{
			_Fill_to_tuple_impl<_Idx>(t, args...);
		}

		template<size_t _Idx>
		struct _Copy_to_tuple_impl
		{
			template<size_t _Offset, class _T1, class _T2>
			static void copy(_T1 & target, _T2&& source)
			{
				std::get<_Offset + _Idx - 1>(target) = std::get<_Idx - 1>(source);
				_Copy_to_tuple_impl<_Idx - 1>::template copy<_Offset>(target, source);
			}			
			template<size_t _Offset, class _T1, class _T2>
			static void move(_T1 & target, _T2&& source)
			{
				std::get<_Offset + _Idx - 1>(target) = std::move(std::get<_Idx - 1>(source));
				_Copy_to_tuple_impl<_Idx - 1>::template move<_Offset>(target, source);
			}
		};
		template<>
		struct _Copy_to_tuple_impl<0>
		{
			template<size_t _Offset, class _T1, class _T2>
			static void copy(_T1 & target, _T2&& source)
			{
			}
			template<size_t _Offset, class _T1, class _T2>
			static void move(_T1 & target, _T2&& source)
			{
			}
		};

		template<size_t _Idx, class _Tuple, class _Tuple2>
		void _Copy_to_tuple(_Tuple & target, _Tuple2 && source)
		{
			static_assert(std::tuple_size_v<_Tuple> >= std::tuple_size_v<_Tuple2> +_Idx, "index is out of range, _Idx must less than t1 size - t2 size");
			_Copy_to_tuple_impl<std::tuple_size_v<_Tuple2>>::copy<_Idx>(target, source);
		}
		template<size_t _Idx, class _Tuple, class _Tuple2>
		void _Move_to_tuple(_Tuple & target, _Tuple2 && source)
		{
			static_assert(std::tuple_size_v<_Tuple> >= std::tuple_size_v<_Tuple2> +_Idx, "index is out of range, _Idx must less than t1 size - t2 size");
			_Copy_to_tuple_impl<std::tuple_size_v<_Tuple2>>::move<_Idx>(target, source);
		}


		//ʹ����һ���ڵ�ķ���ֵ����Ϊ��һ���ڵ�Ĳ���������һ������ڵ�
		//�����һ���ڵ�ķ���ֵ��std::tuple<...>������tupleΪ���ģ��
		template<class _PrevArgs>
		struct _Invoke_then_impl
		{
			template<class _Rtype>
			struct unpack_tuple_node
			{
				using type = task_node<_Rtype, _PrevArgs>;
			};
			template<class _Rtype>
			struct unpack_tuple_cbnode
			{
				using type = task_cbnode<_Rtype, _PrevArgs>;
			};
			template<class _Rtype>
			struct unpack_tuple_function
			{
				using type = std::function<_Rtype(_PrevArgs)>;
			};

			template<class _Fx>
			static decltype(auto) Invoke(_Fx & f, const _PrevArgs & args)
			{
				return f(args);
			}
			template<class _Fx>
			static decltype(auto) Invoke(_Fx & f, _PrevArgs && args)
			{
				return f(std::forward<_PrevArgs>(args));
			}
		};
		template<>
		struct _Invoke_then_impl<void>
		{
			template<class _Rtype>
			struct unpack_tuple_node
			{
				using type = task_node<_Rtype>;
			};
			template<class _Rtype>
			struct unpack_tuple_cbnode
			{
				using type = task_cbnode<_Rtype>;
			};
			template<class _Rtype>
			struct unpack_tuple_function
			{
				using type = std::function<_Rtype()>;
			};

			template<class _Fx>
			static decltype(auto) Invoke(_Fx & f)
			{
				return f(args);
			}
		};
		template<class... _PrevArgs>
		struct _Invoke_then_impl<std::tuple<_PrevArgs...>>
		{
			using tuple_type = std::tuple<_PrevArgs...>;

			template<class _Rtype>
			struct unpack_tuple_node
			{
				using type = task_node<_Rtype, _PrevArgs...>;
			};
			template<class _Rtype>
			struct unpack_tuple_cbnode
			{
				using type = task_cbnode<_Rtype, _PrevArgs...>;
			};
			template<class _Rtype>
			struct unpack_tuple_function
			{
				using type = std::function<_Rtype(_PrevArgs...)>;
			};

			template<class _Fx>
			static decltype(auto) Invoke(_Fx & f, const tuple_type & args)
			{
				return std::apply(f, args);
			}
			template<class _Fx>
			static decltype(auto) Invoke(_Fx & f, tuple_type && args)
			{
				return std::apply(f, std::forward<tuple_type>(args));
			}
		};

		template<class _Fx, class _PrevArgs>
		void _Invoke_then(_Fx & f, _PrevArgs && val)
		{
			using value_type = std::remove_reference_t<_PrevArgs>;
			_Invoke_then_impl<value_type>::template Invoke<_Fx>(f, std::forward<_PrevArgs>(val));
		}
		template<class _Rtype, class _PrevArgs>
		using unpack_tuple_fn_t = typename _Invoke_then_impl<_PrevArgs>::template unpack_tuple_function<_Rtype>::type;
		template<class _Rtype, class _PrevArgs>
		using unpack_tuple_node_t = typename _Invoke_then_impl<_PrevArgs>::template unpack_tuple_node<_Rtype>::type;
		template<class _Rtype, class _PrevArgs>
		using unpack_tuple_cbnode_t = typename _Invoke_then_impl<_PrevArgs>::template unpack_tuple_cbnode<_Rtype>::type;


		template<class _Stype>
		struct _Set_then_helper
		{
			std::shared_ptr<_Stype> _Next;

			template<class... _Args>
			auto operator ()(_Args&&... args) const
			{
				if (_Next->invoke_thiz(std::forward<_Args>(args)...))
					_Next->invoke_then_if();
			}
		};

		template<class _Context, class _Stype>
		struct _Set_then_ctx_helper
		{
			_Context * _Ctx;
			std::shared_ptr<_Stype> _Next;

			template<class... _Args>
			auto operator ()(_Args&&... args) const
			{
				using executor_type = task_executor<_Stype>;
				auto exe = std::make_shared<executor_type>(_Next, std::forward<_Args>(args)...);

				_Ctx->add(exe);
			}
		};

		template<class _Tuple>
		struct package_tuple
		{
			using type = std::tuple<_Tuple>;
		};
		template<class... _Args>
		struct package_tuple<std::tuple<_Args...>>
		{
			using type = std::tuple<_Args...>;
		};
		template<class _Tuple>
		using package_tuple_t = typename package_tuple<_Tuple>::type;
	}
}
