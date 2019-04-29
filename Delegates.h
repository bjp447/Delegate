
#ifndef _DELEGATE_
#define _DELEGATE_
#include "DelegateDetails.h"

//#include <functional>
#include <vector>

#define INDEX_NONE -1

//* Use for a context based delegate name.
//* First argument is the disired delegate name, followed by any amount of parameters.
//* Format: (DelegateName, type (optional), ...)
#define SINGLE_CAST_DELEGATE(DelegateName, ... ) \
	using DelegateName = DLG::SingleCastDelegate<void, __VA_ARGS__>;

//* Use for a context based delegate name.
//* First argument is the disired delegate name, followed by any amount of parameters.
//* Format: (return type, DelegateName, type (optional), ...)
#define SINGLE_CAST_DELEGATE_RetVal(RetT, DelegateName, ... ) \
	using DelegateName = DLG::SingleCastDelegate<RetT, __VA_ARGS__>;

//* Use for a context based delegate name.
//* First argument is the disired delegate name, followed by any amount of parameters.
//* Format: (DelegateName, type (optional), ...)
#define MULTI_CAST_DELEGATE(DelegateName, ... ) \
	using DelegateName = DLG::MultiCastDelegate<__VA_ARGS__>;

namespace DLG
{
	//* Format: <'return type' = void, 'arguement type' (optional), ...>
	template <typename RetT, typename... ParamsT>
	class SingleCastDelegate
	{
	private:
		using FreeFunc = RetT(*)(ParamsT...);
		using LambdaFunc_NoState = FreeFunc;
		//using LambdaFunc_State = std::function< RetType(Params...)>;

	private:
		DLG_Details::DelHandlerInterface<RetT>* s;

	public:
		SingleCastDelegate(): s(nullptr)
		{
			static_assert(std::is_void<RetT>::value == true || std::is_default_constructible<RetT>::value == true
				, "Return type for a delegate must be default constructable.\n");
		}

		virtual ~SingleCastDelegate()
		{
			delete this->s;
		}

		//* Binds smart pointer to class and its method.
		template <class ClassT, typename... ArgsT>
		void Bind(const std::shared_ptr<ClassT> target, RetT(ClassT::*func)(ParamsT...), ArgsT... in)
		{
			delete this->s;
			this->s = DLG_Details::make_MemberDel<RetT>(target, func, std::tuple<ParamsT...>(), std::tuple<ArgsT...>(in...));
		}

		//* Binds smart pointer to class and its const method.
		template <class ClassT, typename... ArgsT>
		void Bind(const std::shared_ptr<ClassT> target, RetT(ClassT::*func)(ParamsT...) const, ArgsT... in)
		{
			delete this->s;
			this->s = DLG_Details::make_MemberDel<RetT>(target, func, std::tuple<ParamsT...>(), std::tuple<ArgsT...>(in...));
		}

		//* Binds free function.
		template <typename... ArgsT>
		void BindFunction(FreeFunc func, ArgsT... in)
		{
			delete this->s;
			this->s = DLG_Details::make_FreeDel<RetT>(func, std::tuple<ParamsT...>(), std::tuple<ArgsT...>(in...));
		}

		//* Binds functor
		template <typename ClassT, typename... ArgsT>
		void BindFunctor(ClassT* const target, ArgsT... in)
		{
			delete this->s;
			static_assert(DLG_Details::Details::Traits::is_Functor<RetT, ClassT, ParamsT...>::value
				, "Object is not a functor or does not properly overload operator() with the paramter or return types specified.\n");
			this->s = DLG_Details::make_RawDel<RetT>(target, &ClassT::operator(), std::tuple<ParamsT...>(), std::tuple<ArgsT...>(in...));
		}

		//* Binds method.
		template <class ClassT, typename... ArgsT>
		void BindRaw(ClassT* const target, RetT(ClassT::*func)(ParamsT...), ArgsT... in)
		{
			delete this->s;
			this->s = DLG_Details::make_RawDel<RetT>(target, func, std::tuple<ParamsT...>(), std::tuple<ArgsT...>(in...));
		}

		//* Binds const method.
		template <class ClassT, typename... ArgsT>
		void BindRaw(ClassT* const target, RetT(ClassT::*func)(ParamsT...) const, ArgsT... in)
		{
			delete this->s;
			this->s = DLG_Details::make_RawDel<RetT>(target, func, std::tuple<ParamsT...>(), std::tuple<ArgsT...>(in...));
		}

		//* UnBinds the methods attached to this delegate.
		void UnBind()
		{
			delete this->s;
			this->s = nullptr;
		}

		//* True if object and method is bound; False, if not.
		bool IsBound() const
		{
			return (this->s == nullptr) ? false : true;
		}

		//* Executes bound functions/methods.
		template<typename... ArgsT>
		RetT Execute(ArgsT... in) const
		{
			if (IsBound() == false)
			{
				std::cerr << "Executing unbound delegate. Pointers will be nullptr else default contructor is called.\n";
				return RetT();
			}

			auto* sp = dynamic_cast<DLG_Details::DelHandler<DLG_Details::TypeGroup<RetT>,
				DLG_Details::TypeGroup<ParamsT...>, DLG_Details::TypeGroup<ArgsT...>>*>(this->s);
			if (sp != nullptr && sp->IsValid() == true) //if calling object is not nullptr
			{
				return sp->Execute(std::forward<ArgsT>(in)...);
			}
			else
			{
				std::cerr << "To many arguements or wrong types to execute. Pointers will be nullptr else default contructor is called.\n";
				return RetT();
			}
		}

		//* Executes bound functions/methods.
		template <typename... ArgsT>
		RetT operator()(ArgsT... in) const
		{
			return this->Execute(std::forward<ArgsT>(in)...);
		}

	};

	//* Format: <'arguement type' (optional), ...>
	template <typename... ParamsT>
	class MultiCastDelegate
	{
	public:
		using RetT = void;
		using params = DLG_Details::TypeGroup<ParamsT...>;

	private:
		using FreeFunc = RetT(*)(ParamsT...);
		using LambdaFunc_NoState = FreeFunc;
		//using LambdaFunc_State = std::function< void(Params...)>;

		template<typename ClassT>
		struct MFSig
		{
			using MemberFunctionSignature = RetT(ClassT::*)(ParamsT...);
			using MemberFunctionConstSignature = RetT(ClassT::*)(ParamsT...) const;
		};

		std::vector<DLG_Details::DelHandlerInterface<RetT>*> Member_Binds;
		//int payLoadAmount;

	public:
		MultiCastDelegate()
		{
			static_assert(std::is_void<RetT>::value == true || std::is_default_constructible<RetT>::value == true
				, "Return type for a delegate must be default constructable.\n");
		}

		~MultiCastDelegate()
		{
			Clear();
		}

		int Size() const
		{
			return this->Member_Binds.size();
		}

	private:
		void RemoveAt(int index)
		{
			if (index >= 0 && index < this->Member_Binds.size())
			{
				auto& back = this->Member_Binds.back();
				std::swap(this->Member_Binds.at(index), back);
				delete back;
				this->Member_Binds.pop_back();
			}
		}

		void Remove(DLG_Details::DelHandlerInterface<RetT>*& bind)
		{
			auto& back = this->Member_Binds.back();
			std::swap(bind, back);
			delete back;
			this->Member_Binds.pop_back();
		}

		//Find free del
		//assums good form on startingIndex.
		int FindBind(FreeFunc func, unsigned startingIndex = 0) const
		{
			return FindBind(nullptr, func, startingIndex);
		}

		//Find member del
		//assums good form on startingIndex.
		template <typename ClassT, typename FuncT>
		int FindBind(ClassT* const target, FuncT func, unsigned startingIndex = 0) const
		{
			for (unsigned i = startingIndex; i < this->Member_Binds.size(); ++i)
			{
				auto bind = Member_Binds[i];
				if (bind != nullptr && target == bind->GetObjectPointer()
					&& DLG_Details::Details::is_equal(func, bind->GetMemberFuncPointer()) == true)
				{
					return i;
				}
			}
			return INDEX_NONE;
		}

		template <typename ClassT, typename FuncT>
		bool _Contains(ClassT* const target, FuncT func) const
		{
			return (FindBind(target, func) != INDEX_NONE);
		}

		template <typename FuncT, typename ClassT, typename... ArgsT>
		void _BindUnique(std::shared_ptr<ClassT>& target, FuncT func, ArgsT... in)
		{
			if (_Contains(target.get(), func) == false)
			{
				this->Member_Binds.push_back(
					DLG_Details::make_MemberDel<RetT>(target, func, std::tuple<ParamsT...>(), std::tuple<ArgsT...>(in...)));
			}
		}

		template <typename ClassT, typename FuncT, typename... ArgsT>
		void _BindUnique(ClassT* const target, FuncT func, ArgsT... in)
		{
			if (_Contains(target, func) == false)
			{
				this->Member_Binds.push_back(
					DLG_Details::make_RawDel<RetT>(target, func, std::tuple<ParamsT...>(), std::tuple<ArgsT...>(in...)));
			}
		}

		template <typename ClassT, typename FuncT, typename... ArgsT>
		void _Bind(std::shared_ptr<ClassT>& target, FuncT func, ArgsT... in)
		{
			this->Member_Binds.push_back(
				DLG_Details::make_MemberDel<RetT>(target, func, std::tuple<ParamsT...>(), std::tuple<ArgsT...>(in...)));
		}

		template <typename FuncT, typename ClassT, typename... ArgsT>
		void _Bind(ClassT* const target, FuncT func, ArgsT... in)
		{
			this->Member_Binds.push_back(
				DLG_Details::make_RawDel<RetT>(target, func, std::tuple<ParamsT...>(), std::tuple<ArgsT...>(in...)));
		}

		template <typename ClassT, typename FuncT>
		void _UnBind(ClassT* const target, FuncT func)
		{
			int index = 0;
			do
			{
				index = FindBind(target, func, index);
				RemoveAt(index);

			} while (index >= 0);
		}

		template <typename FuncT, typename ClassT>
		void _UnBindSingle(ClassT* const target, FuncT func)
		{
			int index = FindBind(target, func);
			RemoveAt(index);
		}

	public:
		//* Calls binded functions. Automatically removes invalid binds.
		template<typename... ArgsT>
		void Broadcast(ArgsT... in)
		{
			for (unsigned i = 0; i < this->Member_Binds.size(); )
			{
				auto& bind = Member_Binds[i];

				if (bind->IsValid())
				{
					auto* sp = dynamic_cast<DLG_Details::DelHandler<DLG_Details::TypeGroup<RetT>, DLG_Details::TypeGroup<ParamsT...>, DLG_Details::TypeGroup<ArgsT...>>*>(bind);
					if (sp != nullptr)
					{
						sp->Execute(std::forward<ArgsT>(in)...);
					}
					else
					{
						std::cerr << "To many arguements or wrong types to execute. Return value may be undifined.\n";
					}
					++i;
				}
				else
				{
					Remove(bind);
				}
			}
		}

		//* Calls binded functions.
		template<typename... ArgsT>
		void Broadcast(ArgsT... in) const
		{
			for (unsigned i = 0; i < this->Member_Binds.size(); ++i)
			{
				auto& bind = Member_Binds[i];

				if (bind->IsValid())
				{
					auto* sp = dynamic_cast<DLG_Details::DelHandler<DLG_Details::TypeGroup<RetT>, DLG_Details::TypeGroup<ParamsT...>, DLG_Details::TypeGroup<ArgsT...>>*>(bind);
					if (sp != nullptr)
					{
						sp->Execute(std::forward<ArgsT>(in)...);
					}
					else
					{
						std::cerr << "To many arguements or wrong types to execute. Return value may be undifined.\n";
					}
				}
			}
		}

		//* Deletes all binds.
		void Clear()
		{
			for (auto& bind : this->Member_Binds)
			{
				delete bind;
			}
			this->Member_Binds.clear();
		}

		//* Broadcast
		template<typename... ArgsT>
		void operator()(ArgsT... in)
		{
			this->Broadcast(std::forward<ArgsT>(in)...);
		}

		//* Binds method provided that it is not already bound.
		template<typename... ArgsT>
		void AddBindUnique(FreeFunc func, ArgsT... in)
		{
			if (ContainsBind(func) == false)
			{
				this->Member_Binds.push_back(
					DLG_Details::make_FreeDel<RetT>(func, std::tuple<ParamsT...>(), std::tuple<ArgsT...>(in...)));
			}
		}

		//* Binds method. Allows duplicates.
		template<typename... ArgsT>
		void AddBind(FreeFunc func, ArgsT... in)
		{
			this->Member_Binds.push_back(
				DLG_Details::make_FreeDel<RetT>(func, std::tuple<ParamsT...>(), std::tuple<ArgsT...>(in...)));
		}

		//* UnBinds the first bind that matches the function signature.
		void RemoveBindSingle(FreeFunc func)
		{
			int index = FindBind(func);
			RemoveAt(index);
		}

		//* UnBinds all methods matching the function signature.
		void RemoveBind(FreeFunc func)
		{
			int index = 0;
			do
			{
				index = FindBind(func, index);
				RemoveAt(index);

			} while (index >= 0);
		}

		//@Return: True if method is bound; False, if not.
		bool ContainsBind(FreeFunc func) const
		{
			return (FindBind(func) != INDEX_NONE);
		}

		//* Binds method. Allows duplicates.
		template <typename ClassT, typename... ArgsT>
		void AddBindUnique(ClassT* const& target, typename MFSig<ClassT>::MemberFunctionSignature func, ArgsT... in)
		{
			_BindUnique(target, func, std::forward<ArgsT>(in)...);
		}

		//* Binds method. Allows duplicates.
		template <typename RetT, typename ClassT, typename... ArgsT>
		void AddBindUnique(ClassT* const& target, typename MFSig<ClassT>::MemberFunctionConstSignature func, ArgsT... in)
		{
			_BindUnique(target, func, std::forward<ArgsT>(in)...);
		}

		//* Binds method. Allows duplicates.
		template <typename ClassT, typename... ArgsT>
		void AddBind(ClassT* const& target, typename MFSig<ClassT>::MemberFunctionSignature func, ArgsT... in)
		{
			_Bind(target, func, std::forward<ArgsT>(in)...);
		}

		//* Binds method. Allows duplicates.
		template <typename ClassT, typename... ArgsT>
		void AddBind(ClassT* const& target, typename MFSig<ClassT>::MemberFunctionConstSignature func, ArgsT... in)
		{
			_Bind(target, func, std::forward<ArgsT>(in)...);
		}

		//* UnBinds all methods matching the function signature and object instance.
		template <typename ClassT>
		void RemoveBind(ClassT* const& target, typename MFSig<ClassT>::MemberFunctionSignature func)
		{
			_UnBind(target, func);
		}

		//* UnBinds all methods matching the function signature and object instance.
		template <typename ClassT>
		void RemoveBind(ClassT* const& target, typename MFSig<ClassT>::MemberFunctionConstSignature func)
		{
			_UnBind(target, func);
		}

		//* UnBinds the first bind that matches the function signature and object instance.
		template <typename ClassT>
		void RemoveBindSingle(ClassT* const& target, typename MFSig<ClassT>::MemberFunctionSignature func)
		{
			_UnBindSingle(target, func);
		}

		//* UnBinds the first bind that matches the function signature and object instance.
		template <typename ClassT>
		void RemoveBindSingle(ClassT* const& target, typename MFSig<ClassT>::MemberFunctionConstSignature func)
		{
			_UnBindSingle(target, func);
		}

		//@Return: True if object and method is bound; False, if not.
		template <typename ClassT>
		bool ContainsBind(ClassT* const& target, typename MFSig<ClassT>::MemberFunctionSignature func) const
		{
			return _Contains(target, func);
		}

		//@Return: True if object and method is bound; False, if not.
		template <typename ClassT>
		bool ContainsBind(ClassT* const& target, typename MFSig<ClassT>::MemberFunctionConstSignature func) const
		{
			return _Contains(target, func);
		}

		template <typename ClassT>
		bool ContainsInstance(ClassT* const& target)
		{
			for (const auto& bind : this->Member_Binds)
			{
				if (bind->GetMemberFuncPointer() == target)
				{
					return true;
				}
			}
			return false;
		}

		//UnBinds all methods of object instance.
		template <typename ClassT>
		void RemoveBindAllInstance(ClassT* const& target)
		{
			//_RemoveBindAllInstance(target);
			for (unsigned i = 0; i < this->Member_Binds.size(); )
			{
				auto& bind = Member_Binds[i];

				if (target == bind->GetObjectPointer())
				{
					Remove(bind);
				}
				else
				{
					++i;
				}
			}
		}

		//* Binds method provided that it is not already bound.
		template <typename ClassT, typename... ArgsT>
		void AddBindUnique(std::shared_ptr<ClassT>& target, typename MFSig<ClassT>::MemberFunctionSignature func, ArgsT... in)
		{
			_BindUnique(target, func, std::forward<ArgsT>(in)...);
		}

		//* Binds method provided that it is not already bound.
		template <typename ClassT, typename... ArgsT>
		void AddBindUnique(std::shared_ptr<ClassT>& target, typename MFSig<ClassT>::MemberFunctionConstSignature func, ArgsT... in)
		{
			_BindUnique(target, func, std::forward<ArgsT>(in)...);
		}

		//* Binds method. Allows duplicates.
		template <typename ClassT, typename... ArgsT>
		void AddBind(std::shared_ptr<ClassT>& target, typename MFSig<ClassT>::MemberFunctionSignature func, ArgsT... in)
		{
			_Bind(target, func, std::forward<ArgsT>(in)...);
		}

		//* Binds method. Allows duplicates.
		template <typename ClassT, typename... ArgsT>
		void AddBind(std::shared_ptr<ClassT>& target, typename MFSig<ClassT>::MemberFunctionConstSignature func, ArgsT... in)
		{
			_Bind(target, func, std::forward<ArgsT>(in)...);
		}

		//* UnBinds all methods matching the function signature and object instance.
		template <typename ClassT>
		void RemoveBind(std::shared_ptr<ClassT>& target, typename MFSig<ClassT>::MemberFunctionSignature func)
		{
			_UnBind(target.get(), func);
		}

		//* UnBinds all methods matching the function signature and object instance.
		template <typename ClassT>
		void RemoveBind(std::shared_ptr<ClassT>& target, typename MFSig<ClassT>::MemberFunctionConstSignature func)
		{
			_UnBind(target.get(), func);
		}

		//* UnBinds the first bind that matches the function signature and object instance.
		template <typename ClassT>
		void RemoveBindSingle(std::shared_ptr<ClassT>& target, typename MFSig<ClassT>::MemberFunctionSignature func)
		{
			_UnBindSingle(target.get(), func);
		}

		//* UnBinds the first bind that matches the function signature and object instance.
		template <typename ClassT>
		void RemoveBindSingle(std::shared_ptr<ClassT>& target, typename MFSig<ClassT>::MemberFunctionConstSignature func)
		{
			_UnBindSingle(target.get(), func);
		}

		//@Return: True if object and method is bound; False, if not.
		template <typename ClassT>
		bool ContainsBind(std::shared_ptr<ClassT>& target, typename MFSig<ClassT>::MemberFunctionSignature func) const
		{ 
			return _Contains(target.get(), func);
		}

		//@Return: True if object and method is bound; False, if not.
		template <typename ClassT>
		bool ContainsBind(std::shared_ptr<ClassT>& target, typename MFSig<ClassT>::MemberFunctionConstSignature func) const
		{ 
			return _Contains(target.get(), func);
		}

		//UnBinds all methods of object instance.
		template <typename ClassT>
		void RemoveBindAllInstance(std::shared_ptr<ClassT>& target)
		{
			RemoveBindAllInstance(target.get());
		}

		template <typename ClassT>
		bool ContainsInstance(std::shared_ptr<ClassT>& target)
		{
			return ContainsInstance(target.get());
		}

	};

}

#endif // !_DELEGATE_
