
#pragma once
#ifndef _DelDets_
#define _DelDets_

#include <tuple>
#include <memory>
#include <type_traits>
#include <typeinfo>

namespace DLG_Details
{
	//* TypeGrouping: wrapper to allow for multiple packings.
	//* retT: return type specified on delegate creation. 
	//* ParamsT: all types specified on delegate creation. 
	//* EArgsT: arguements at execute.
	//* BArgsT: payload arguements at bind.

	//* struct for allowing for multiple patckings in a templated class.
	template<typename... Types> struct TypeGroup {};

	//* DelHander <template> bases.
	template<typename...> struct DelHandler; //used during execution.
	template<typename...> struct FreeDelHandler; //created by maker during bindings.
	template<typename...> struct MemberDelHandler;
	template<typename...> struct RawDelHandler;
	
	/*
			   A
			   |
			   B
			 /   \_______
			|     |     |
			E     F     G
	*/

	//*DelHander specializations*//

	//A. used when storing a delegate handler
	template <typename RetT> struct DelHandlerInterface;
	//B. casted on Execute
	template<template<typename...> typename TypeGrouping, typename RetT, typename... ParamsT, typename... EArgsT>
	struct DelHandler<TypeGrouping<RetT>, TypeGrouping<ParamsT...>, TypeGrouping<EArgsT...>>;
	//E. constructed on bind
	template<template<typename...> typename TypeGrouping, typename RetT, typename FuncT, typename... ParamsT, typename... EArgsT, typename... BArgsT>
	struct FreeDelHandler<TypeGrouping<RetT, FuncT, ParamsT...>, TypeGrouping<EArgsT...>, TypeGrouping<BArgsT...>>;
	//F. constructed on bind
	template<template<typename...> typename TypeGrouping, typename RetT, typename ClassT, typename FuncT, typename... ParamsT, typename... EArgsT, typename... BArgsT>
	struct MemberDelHandler<TypeGrouping<RetT, ClassT, FuncT, ParamsT...>, TypeGrouping<EArgsT...>, TypeGrouping<BArgsT...>>;
	//G. constructed on bind
	template<template<typename...> typename TypeGrouping, typename RetT, typename ClassT, typename FuncT, typename... ParamsT, typename... EArgsT, typename... BArgsT>
	struct RawDelHandler<TypeGrouping<RetT, ClassT, FuncT, ParamsT...>, TypeGrouping<EArgsT...>, TypeGrouping<BArgsT...>>;

	//* Base used as reference in actual delegate classes. instantiated only as a base.
	template <typename RetT>
	struct DelHandlerInterface
	{
	protected:
		DelHandlerInterface() {}
	public:
		virtual ~DelHandlerInterface() {};

		virtual bool IsValid() const { return true; }
		virtual const void* const GetObjectPointer() const { return nullptr; }
		virtual const void* GetMemberFuncPointer() const { return 0; }
	};

	//* DelHolder2, never instantiated alone. Used during Execute()
	template<template<typename...> typename TypeGrouping, typename RetT, typename... ParamsT, typename... EArgsT>
	struct DelHandler<TypeGrouping<RetT>, TypeGrouping<ParamsT...>, TypeGrouping<EArgsT...>>
		: public DelHandlerInterface<RetT>
	{
	protected:
		DelHandler<TypeGrouping<RetT>, TypeGrouping<ParamsT...>, TypeGrouping<EArgsT...>>()
			: DelHandlerInterface<RetT>() {}
		virtual ~DelHandler<TypeGrouping<RetT>, TypeGrouping<ParamsT...>, TypeGrouping<EArgsT...>>() {};

	public:
		virtual RetT Execute(EArgsT... in) const = 0;
	};

	//* FreeDelHandler, the one always instantiated. Used during Binding()
	template<template<typename...> typename TypeGrouping, typename RetT, typename FuncT, typename... ParamsT, typename... EArgsT, typename... BArgsT>
	struct FreeDelHandler<TypeGrouping<RetT, FuncT, ParamsT...>, TypeGrouping<EArgsT...>, TypeGrouping<BArgsT...>> final
		: public DelHandler<TypeGrouping<RetT>, TypeGrouping<ParamsT...>, TypeGrouping<EArgsT...>>
	{
	private:
		FuncT Function;
		std::tuple<BArgsT...> t;

	public:
		FreeDelHandler<TypeGrouping<RetT, FuncT, ParamsT...>, TypeGrouping<EArgsT...>, TypeGrouping<BArgsT...>>
			(FuncT func, const std::tuple<BArgsT...>& t) 
			: DelHandler<TypeGrouping<RetT>, TypeGrouping<ParamsT...>, TypeGrouping<EArgsT...>>()
			, Function(func), t(t) {}

		virtual ~FreeDelHandler<TypeGrouping<RetT, FuncT, ParamsT...>, TypeGrouping<EArgsT...>, TypeGrouping<BArgsT...>>() {}

		virtual RetT Execute(EArgsT... in) const override final
		{
			return Details::apply(this->Function
				, std::tuple_cat(std::tuple<EArgsT...>(in...), this->t));
		}

		virtual FuncT GetFunctionPointer() const
		{
			return this->Function;
		}

		virtual const void* GetMemberFuncPointer() const override final
		{
			//return typeid(FuncT).hash_code();
			return Details::void_cast(this->Function);
		}
	};

	//* MemberDelHandler, the one always instantiated. Used during Binding()
	template<template<typename...> typename TypeGrouping, typename RetT, typename ClassT, typename FuncT, typename... ParamsT, typename... EArgsT, typename... BArgsT>
	struct MemberDelHandler<TypeGrouping<RetT, ClassT, FuncT, ParamsT...>, TypeGrouping<EArgsT...>, TypeGrouping<BArgsT...>> final
		: public DelHandler<TypeGrouping<RetT>, TypeGrouping<ParamsT...>, TypeGrouping<EArgsT...>>
	{
	private:
		std::weak_ptr<ClassT> Object;
		FuncT Function;
		std::tuple<BArgsT...> t;

	public:
		MemberDelHandler<TypeGrouping<RetT, ClassT, FuncT, ParamsT...>, TypeGrouping<EArgsT...>, TypeGrouping<BArgsT...>>
			(std::shared_ptr<ClassT> target, FuncT func, const std::tuple<BArgsT...>& t) 
			: DelHandler<TypeGrouping<RetT>, TypeGrouping<ParamsT...>, TypeGrouping<EArgsT...>>()
			, Object(target), Function(func), t(t) {}

		virtual ~MemberDelHandler<TypeGrouping<RetT, ClassT, FuncT, ParamsT...>, TypeGrouping<EArgsT...>, TypeGrouping<BArgsT...>>() 
		{
			this->Object.reset();
		}

		virtual RetT Execute(EArgsT... in) const override final
		{
			return Details::apply(this->Object.lock().get(), this->Function
				, std::tuple_cat(std::tuple<EArgsT...>(in...), this->t));
		}

		virtual bool IsValid() const override final
		{
			return !this->Object.expired();
		}

		virtual const void* const GetObjectPointer() const override final
		{
			return static_cast<void*>(this->Object.lock().get());
		}

		virtual FuncT GetFunctionPointer() const
		{
			return this->Function;
		}

		virtual const void*/*std::size_t*/ GetMemberFuncPointer() const override final
		{
			//return typeid(FuncT).hash_code();
			return Details::void_cast(this->Function);
		}
	};

	//* RawDelHandler, the one always instantiated. Used during Binding()
	template<template<typename...> typename TypeGrouping, typename RetT, typename ClassT, typename FuncT, typename... ParamsT, typename... EArgsT, typename... BArgsT>
	struct RawDelHandler<TypeGrouping<RetT, ClassT, FuncT, ParamsT...>, TypeGrouping<EArgsT...>, TypeGrouping<BArgsT...>> final
		: public DelHandler<TypeGrouping<RetT>, TypeGrouping<ParamsT...>, TypeGrouping<EArgsT...>>
	{
	private:
		ClassT* Object;
		FuncT Function;
		std::tuple<BArgsT...> t;

	public:
		RawDelHandler<TypeGrouping<RetT, ClassT, FuncT, ParamsT...>, TypeGrouping<EArgsT...>, TypeGrouping<BArgsT...>>
			(ClassT* target, FuncT func, const std::tuple<BArgsT...>& t) 
			: DelHandler<TypeGrouping<RetT>, TypeGrouping<ParamsT...>, TypeGrouping<EArgsT...>>()
			, Object(target), Function(func), t(t) {}

		virtual ~RawDelHandler<TypeGrouping<RetT, ClassT, FuncT, ParamsT...>, TypeGrouping<EArgsT...>, TypeGrouping<BArgsT...>>() {}

		virtual RetT Execute(EArgsT... in) const override final
		{
			return Details::apply(this->Object, this->Function
				, std::tuple_cat(std::tuple<EArgsT...>(in...), this->t));
		}

		virtual bool IsValid() const override final
		{
			return (this->Object != nullptr);
		}

		virtual const void* const GetObjectPointer() const override final
		{
			return static_cast<void*>(this->Object);
		}

		virtual FuncT GetFunctionPointer() const
		{
			return this->Function;
		}

		virtual const void* GetMemberFuncPointer() const
		{
			//return typeid(FuncT).hash_code();
			return Details::void_cast(this->Function);
		}
	};

	//* makeDel namespace
	namespace _make
	{
		//* Creates delegate handler with split paramter pack. Split between the payload types and the rest of the types.
		template<typename RetT, typename ClassT, typename FuncT, typename... ParamsT, typename... EArgsT, typename... BArgsT>
		DelHandlerInterface<RetT>* _makeMemberDel_Impl(const std::shared_ptr<ClassT>& target, FuncT func,
			const std::tuple<ParamsT...>& allT, const std::tuple<EArgsT...>& params, const std::tuple<BArgsT...>& args)
		{
			return new MemberDelHandler<TypeGroup<RetT, ClassT, FuncT, ParamsT...>
				, TypeGroup<EArgsT...>, TypeGroup<BArgsT...>>(target, func, args);
		}

		//* Creates delegate handler with split paramter pack. Split between the payload types and the rest of the types.
		template<typename RetT, typename ClassT, typename FuncT, typename... ParamsT, typename... EArgsT, typename... BArgsT>
		DelHandlerInterface<RetT>* _makeRawDel_Impl(ClassT* const target, FuncT func,
			const std::tuple<ParamsT...>& allT, const std::tuple<EArgsT...>& params, const std::tuple<BArgsT...>& args)
		{
			return new RawDelHandler<TypeGroup<RetT, ClassT, FuncT, ParamsT...>
				, TypeGroup<EArgsT...>, TypeGroup<BArgsT...>>(target, func, args);
		}
		
		//* Creates delegate handler with split paramter pack. Split between the payload types and the rest of the types.
		template<typename RetT, typename FuncT, typename... ParamsT, typename... EArgsT, typename... BArgsT>
		DelHandlerInterface<RetT>* _makeFreeDel_Impl(FuncT func,
			const std::tuple<ParamsT...>&, const std::tuple<EArgsT...>&, const std::tuple<BArgsT...>& args)
		{
			return new FreeDelHandler<TypeGroup<RetT, FuncT, ParamsT...>
				, TypeGroup<EArgsT...>, TypeGroup<BArgsT...>>(func, args);
		}

		//* Asserts aguement size and equality and returns truncated paramter type list from Bs.
		//* Helper function for make_dels.
		template<typename... As, typename... Bs>
		constexpr decltype(auto) getArgs(std::tuple<As...> allTuple, std::tuple<Bs...> payLoad)
		{
			static_assert(sizeof...(As) >= sizeof...(Bs), "To many arguements in Bind.\n"); //error check
			//Details::Traits::assert_size_gt(allTuple, payLoad);
			const auto params = Details::TupleDetails::take_front<sizeof...(Bs)>(allTuple);
			Details::Traits::assert_packs_same(allTuple, std::tuple_cat(params, payLoad)); //error check
			return params;
		}
	}

	/*
	* these exist because bind types and execute types need to be split from all paramters
	* where _..._Impl functions use the deduced execution arg types to construct delegate handlers.
	*/
	namespace
	{
		//* Creates delegate handler with split paramter pack. Split between the payload types and the rest of the types.
		template<typename RetT, typename ClassT, typename FuncT, typename... ParamsT, typename... BArgsT>
		DelHandlerInterface<RetT>* make_MemberDel(const std::shared_ptr<ClassT>& target, FuncT func,
			const std::tuple<ParamsT...>& allTuple, const std::tuple<BArgsT...>& payLoad)
		{
			return _make::_makeMemberDel_Impl<RetT>(target, func, allTuple, _make::getArgs(allTuple, payLoad), payLoad);
		}

		//* Creates delegate handler with split paramter pack. Split between the payload types and the rest of the types.
		template<typename RetT, typename ClassT, typename FuncT, typename... ParamsT, typename... BArgsT>
		DelHandlerInterface<RetT>* make_RawDel(ClassT* target, FuncT func,
			const std::tuple<ParamsT...>& allTuple, const std::tuple<BArgsT...>& payLoad)
		{
			return _make::_makeRawDel_Impl<RetT>(target, func, allTuple, _make::getArgs(allTuple, payLoad), payLoad);
		}

		//* Creates delegate handler with split paramter pack. Split between the payload types and the rest of the types.
		template<typename RetT, typename FuncT, typename... ParamsT, typename... BArgsT>
		DelHandlerInterface<RetT>* make_FreeDel(FuncT func,
			const std::tuple<ParamsT...>& allTuple, const std::tuple<BArgsT...>& payLoad)
		{
			return _make::_makeFreeDel_Impl<RetT>(func, allTuple, _make::getArgs(allTuple, payLoad), payLoad);
		}
	}

	namespace Details
	{
		namespace FunctionCasting
		{
			template<typename RetT, typename... ParamsT>
			const void* const void_cast(RetT(*func)(ParamsT...))
			{
				union {
					RetT(*pf)(ParamsT...);
					void* p;
				};
				pf = func;
				return p;
			}
			
			template<typename RetT, typename ClassT, typename... ParamsT>
			const void* const void_cast(RetT(ClassT::*func)(ParamsT...))
			{
				union {
					RetT(ClassT::*pf)(ParamsT...);
					void* p;
				};
				pf = func;
				return p;
			}
			
			template<typename RetT, typename ClassT, typename... ParamsT>
			const void* const void_cast(RetT(ClassT::*func)(ParamsT...) const)
			{
				union {
					RetT(ClassT::*pf)(ParamsT...) const;
					void* p;
				};
				pf = func;
				return p;
			}

			template<typename RetT, typename... ParamsT>
			bool is_equal(RetT(*func1)(ParamsT...), const void* func2)
			{
				return (void_cast(func1) == func2);
			}

			template<typename RetT, typename ClassT, typename... ParamsT>
			bool is_equal(RetT(ClassT::*func1)(ParamsT...), const void* func2)
			{
				return (void_cast(func1) == func2);
			}

			template<typename RetT, typename ClassT, typename... ParamsT>
			bool is_equal(RetT(ClassT::*func1)(ParamsT...) const, const void* func2)
			{
				return (void_cast(func1) == func2);
			}
			//template<typename RetT, typename... ParamsT>
			//bool is_equal(RetT(*func1)(ParamsT...), const std::size_t& func2)
			//{
			//	std::size_t s = typeid(RetT(*)(ParamsT...)).hash_code();
			//	return (s == func2);
			//}
			//
			//template<typename RetT, typename ClassT, typename... ParamsT>
			//bool is_equal(RetT(ClassT::*func1)(ParamsT...), const std::size_t& func2)
			//{
			//	std::size_t s = typeid(RetT(ClassT::*)(ParamsT...)).hash_code();
			//	return (s == func2);
			//}
			//
			//template<typename RetT, typename ClassT, typename... ParamsT>
			//bool is_equal(RetT(ClassT::*func1)(ParamsT...) const, const std::size_t& func2)
			//{
			//		std::size_t s = typeid(RetT(ClassT::*)(ParamsT...) const).hash_code();
			//		return (s == func2);
			//}
		}

		namespace Traits
		{
			template<typename...> struct are_args_same;

			//* Checks that T is a functor with specified types.
			template <typename ReturnT, typename ClassT, typename... ArgsT>
			struct is_Functor final
			{
			private:
				struct BadType final {};
				template <class U> static decltype(std::declval<U>().operator()(std::declval<ArgsT>()...)) Test(int);
				template <class U> static BadType Test(...);
			public:
				static constexpr bool value = std::is_convertible<decltype(Test<ClassT>(0)), ReturnT>::value;
			};

			//* Checks that two parameter packs are equal.
			template<template<typename...> typename TypeGrouping, typename... As, typename... Bs>
			struct are_args_same<TypeGrouping<As...>, TypeGrouping<Bs...>> final
			{
			private:
				template <typename, typename> struct ArgCheck final : std::false_type {};
				template<> struct ArgCheck<TypeGroup<>, TypeGroup<>> final : std::true_type {};

				template <typename A, typename... As, typename B, typename... Bs>
				struct ArgCheck<TypeGroup<A, As...>, TypeGroup<B, Bs...>> final
					: std::integral_constant<bool, std::is_same<A, B>::value
					&& ArgCheck<TypeGroup<As...>, TypeGroup<Bs...>>::value> {};
			public:
				static constexpr bool value = ArgCheck<TypeGroup<As...>, TypeGroup<Bs...>>::value;
			};

			//* Checks that two parameter packs are equal.
			template<typename... As, typename... Bs>
			void assert_packs_same(const std::tuple<As...>&, const std::tuple<Bs...>&)
			{
				static_assert(Details::Traits::are_args_same<TypeGroup<As...>, TypeGroup<Bs...>>::value,
					"Argument types do not match function signature.");
			}

			template<typename... As, typename... Bs>
			void assert_size_gt(const std::tuple<As...>& params, const std::tuple<Bs...>& args)
			{
				static_assert(sizeof...(As) >= sizeof...(Bs), "To many arguements in Bind"); 
			}
		}

		namespace TupleDetails
		{
			//* Take_front helper.
			template <typename... ParamsT, size_t... I>
			constexpr decltype(auto) take_front_impl(const std::tuple<ParamsT...>& t, const std::index_sequence<I...>&)
			{
				return std::make_tuple(std::get<I>(t)...);
			}

			//* Creates a tuple with 'Trim' less arguements.
			template <size_t Trim, typename... ParamsT>
			constexpr decltype(auto) take_front(const std::tuple<ParamsT...>& t)
			{
				return take_front_impl(t, std::make_index_sequence<sizeof...(ParamsT) - Trim>());
			}
			//------------------------------------------

			//* Apply free function helper.
			template <typename FreeFunc, typename... ParamsT, size_t... I>
			constexpr decltype(auto) apply_impl(const FreeFunc& f, const std::tuple<ParamsT...>& t, const std::index_sequence<I...>&)
			{
				return f(std::get<I>(t)...);
			}

			//* Calls free function with decupled tuple args.  
			template <typename FreeFunc, typename... ParamsT>
			constexpr decltype(auto) apply(const FreeFunc& f, const std::tuple<ParamsT...>& t)
			{
				return apply_impl(f, t, std::make_index_sequence<sizeof...(ParamsT)>());
			}
			//------------------------------------------

			//* Apply member function helper.
			template <typename ClassT, typename FreeFunc, typename... ParamsT, size_t... I>
			constexpr decltype(auto) apply_impl(ClassT obj, const FreeFunc& f, const std::tuple<ParamsT...>& t, const std::index_sequence<I...>&)
			{
				return (obj.*f)(std::get<I>(t)...);
			}

			//* Calls member function with decupled tuple args.  
			template <typename ClassT, typename FuncT, typename... ParamsT>
			constexpr decltype(auto) apply(ClassT obj, const FuncT& f, const std::tuple<ParamsT...>& t)
			{
				return apply_impl(obj, f, t, std::make_index_sequence<sizeof...(ParamsT)>());
			}
			//------------------------------------------

			//* Apply member function helper.
			template <typename ClassT, typename FreeFunc, typename... ParamsT, size_t... I>
			constexpr decltype(auto) apply_impl(ClassT* obj, const FreeFunc& f, const std::tuple<ParamsT...>& t, const std::index_sequence<I...>&)
			{
				return ((*obj).*f)(std::get<I>(t)...);
			}

			//* Calls member function with decupled tuple args.  
			template <typename ClassT, typename FuncT, typename... ParamsT>
			constexpr decltype(auto) apply(ClassT* obj, const FuncT& f, const std::tuple<ParamsT...>& t)
			{
				return apply_impl(obj, f, t, std::make_index_sequence<sizeof...(ParamsT)>());
			}
		}

		using namespace FunctionCasting;
		using namespace TupleDetails;
	}
}

#endif // !_DelDets_
