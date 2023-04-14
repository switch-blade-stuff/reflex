/*
 * Created by switchblade on 2023-03-13.
 */

#pragma once

#include <array>
#include <list>

#include "../delegate.hpp"
#include "spinlock.hpp"
#include "any.hpp"

namespace reflex
{
	namespace detail
	{
		std::size_t type_hash::operator()(const type_info &value) const { return tpp::seahash_hash<type_info>{}(value); }

		bool type_eq::operator()(const type_info &a, const type_info &b) const { return a == b; }
		bool type_eq::operator()(const std::string &a, const type_info &b) const { return std::string_view{a} == b; }
		bool type_eq::operator()(const type_info &a, const std::string &b) const { return a == std::string_view{b}; }
		bool type_eq::operator()(const type_info &a, const std::string_view &b) const { return a == b; }
		bool type_eq::operator()(const std::string_view &a, const type_info &b) const { return a == b; }

		using vtab_table = tpp::dense_map<std::string_view, const void *>;
		using attr_table = tpp::dense_map<std::string_view, any, type_hash, type_eq>;
		using enum_table = tpp::dense_map<std::string, any, type_hash, type_eq>;

		struct type_base
		{
			type_handle type;
			base_cast cast_func;
		};

		using base_table = tpp::dense_map<std::string_view, type_base>;

		template<typename T, typename B>
		[[nodiscard]] inline static type_base make_type_base() noexcept
		{
			constexpr auto cast = [](const void *ptr) noexcept -> const void *
			{
				auto *base = static_cast<std::add_const_t<B> *>(static_cast<std::add_const_t<T> *>(ptr));
				return static_cast<const void *>(base);
			};
			return {data_factory<B>, cast};
		}

		struct type_conv
		{
			type_conv() noexcept = default;
			template<typename From, typename To, typename F>
			type_conv(std::in_place_type_t<From>, std::in_place_type_t<To>, F &&conv)
			{
				if constexpr (!std::is_invocable_r_v<any, F, const void *>)
					func = [f = std::forward<F>(conv)](const void *data) { return make_any<To>(std::invoke(f, *static_cast<const From *>(data))); };
				else
					func = std::forward<F>(conv);
			}

			[[nodiscard]] any operator()(const void *data) const { return func(data); }

			delegate<any(const void *)> func;
		};

		using conv_table = tpp::dense_map<std::string_view, type_conv>;

		template<typename From, typename To, typename F>
		[[nodiscard]] inline static type_conv make_type_conv(F &&conv)
		{
			return {std::in_place_type<From>, std::in_place_type<To>, std::forward<F>(conv)};
		}
		template<typename From, typename To>
		[[nodiscard]] inline static type_conv make_type_conv() noexcept
		{
			return make_type_conv<From, To>([](auto &value) { return static_cast<To>(value); });
		}

		struct arg_data
		{
			[[nodiscard]] static bool match_exact(const auto &a, const auto &b)
			{
				return std::ranges::equal(a, b, [](auto &&a, auto &&b) { return a == b; });
			}
			[[nodiscard]] static bool match_compatible(const auto &a, const auto &b, database_impl &db)
			{
				return std::ranges::equal(a, b, [&](auto &&a, auto &&b) { return a.compatible(b, db); });
			}

			constexpr arg_data() noexcept = default;
			template<typename T>
			constexpr arg_data(std::in_place_type_t<T>) noexcept : type_name(type_name_v<std::decay_t<T>>), type(data_factory<std::decay_t<T>>)
			{
				if constexpr (!std::is_lvalue_reference_v<T>) flags |= is_value;
				if constexpr (std::is_const_v<std::remove_reference_t<T>>) flags |= is_const;
			}

			[[nodiscard]] inline bool compatible(const any &other, database_impl &db) const;
			[[nodiscard]] inline bool compatible(const arg_data &other, database_impl &db) const;

			[[nodiscard]] friend constexpr bool operator==(const arg_data &a, const any &b) noexcept
			{
				return static_cast<int>(a.flags) == ((b.is_ref() << 1) | int{b.is_const()}) && a.type_name == b.type().name();
			}
			[[nodiscard]] friend constexpr bool operator==(const arg_data &a, const arg_data &b) noexcept
			{
				return a.flags == b.flags && a.type_name == b.type_name;
			}

			/* Type name is cached separately in order to enable quick comparisons. */
			std::string_view type_name;
			type_flags flags = {};
			type_handle type = {};
		};

		template<typename T>
		[[nodiscard]] static arg_data make_arg_data() noexcept { return arg_data{std::in_place_type<T>}; }
		template<typename... Ts>
		[[nodiscard]] inline static std::span<const arg_data> make_argument_view() noexcept
		{
			if constexpr (sizeof...(Ts) != 0)
			{
				static auto value = std::array{make_arg_data<Ts>()...};
				return {value};
			}
			return {};
		}

		struct type_ctor
		{
			template<typename T, typename U = std::remove_reference_t<T>>
			using forward_arg_t = std::conditional_t<std::is_reference_v<T>, U, std::add_const_t<U>>;

			template<typename T, typename... Ts, typename F, std::size_t... Is>
			[[nodiscard]] inline static decltype(auto) construct(std::index_sequence<Is...>, F &&f, std::span<any> args)
			{
				return std::invoke(f, (*args[Is].cast<Ts>().template try_get<forward_arg_t<Ts>>())...);
			}
			template<typename T, typename... Ts, typename F>
			[[nodiscard]] inline static decltype(auto) construct(F &&f, std::span<any> args)
			{
				return construct<T, Ts...>(std::make_index_sequence<sizeof...(Ts)>{}, std::forward<F>(f), args);
			}

			type_ctor() noexcept = default;
			template<typename T, typename... Ts, typename F>
			type_ctor(std::in_place_type_t<T>, type_pack_t<Ts...>, F &&ctor) : args(make_argument_view<Ts...>())
			{
				if constexpr (!std::is_invocable_r_v<any, F, std::span<any>>)
					func = [f = std::forward<F>(ctor)](std::span<any> any_args)
					{
						auto result = construct<T, Ts...>(f, any_args);
						if constexpr (!std::same_as<std::remove_cvref_t<decltype(result)>, any>)
							return forward_any(result);
						else
							return result;
					};
				else
					func = std::forward<F>(ctor);
			}

			[[nodiscard]] any operator()(std::span<any> arg_vals) const { return func(arg_vals); }

			delegate<any(std::span<any>)> func;
			std::span<const arg_data> args;
		};

		template<typename T, typename... Ts, typename F>
		[[nodiscard]] inline static type_ctor make_type_ctor(F &&func)
		{
			return {std::in_place_type<T>, type_pack<Ts...>, std::forward<F>(func)};
		}
		template<typename T, typename... Ts>
		[[nodiscard]] inline static type_ctor make_type_ctor() noexcept
		{
			return make_type_ctor<T, Ts...>([](Ts ...as) -> any { return make_any<T>(as...); });
		}

		struct type_cmp
		{
			bool (*cmp_eq)(const void *, const void *) = nullptr;
			bool (*cmp_ne)(const void *, const void *) = nullptr;
			bool (*cmp_ge)(const void *, const void *) = nullptr;
			bool (*cmp_le)(const void *, const void *) = nullptr;
			bool (*cmp_gt)(const void *, const void *) = nullptr;
			bool (*cmp_lt)(const void *, const void *) = nullptr;
		};

		using cmp_table = tpp::dense_map<std::string_view, type_cmp>;

#ifdef _MSC_VER
		/* Allow signed/unsigned comparison. */
#pragma warning(push)
#pragma warning(disable : 4018)
#endif
		template<typename T, typename U = T>
		[[nodiscard]] inline static type_cmp make_type_cmp() noexcept
		{
			type_cmp result;
			if constexpr (requires(const T &a, const U &b){ a == b; })
				result.cmp_eq = +[](const void *a, const void *b) { return *static_cast<const T *>(a) == *static_cast<const U *>(b); };
			if constexpr (requires(const T &a, const U &b){ a != b; })
				result.cmp_ne = +[](const void *a, const void *b) { return *static_cast<const T *>(a) != *static_cast<const U *>(b); };
			if constexpr (requires(const T &a, const U &b){ a >= b; })
				result.cmp_ge = +[](const void *a, const void *b) { return *static_cast<const T *>(a) >= *static_cast<const U *>(b); };
			if constexpr (requires(const T &a, const U &b){ a <= b; })
				result.cmp_le = +[](const void *a, const void *b) { return *static_cast<const T *>(a) <= *static_cast<const U *>(b); };
			if constexpr (requires(const T &a, const U &b){ a > b; })
				result.cmp_gt = +[](const void *a, const void *b) { return *static_cast<const T *>(a) > *static_cast<const U *>(b); };
			if constexpr (requires(const T &a, const U &b){ a < b; })
				result.cmp_lt = +[](const void *a, const void *b) { return *static_cast<const T *>(a) < *static_cast<const U *>(b); };
			return result;
		}
#ifdef _MSC_VER
#pragma warning(pop)
#endif

		struct any_funcs_t
		{
			void (any::*copy_init)(type_info, const void *, void *) = nullptr;
			void (any::*copy_assign)(type_info, const void *, void *) = nullptr;
		};

		template<typename T>
		[[nodiscard]] constexpr static any_funcs_t make_any_funcs() noexcept
		{
			any_funcs_t result;
			result.copy_init = &any::copy_from<T>;
			result.copy_assign = &any::assign_from<T>;
			return result;
		}

		/* Mutable type data initialized at runtime. Requires spinlock, as it can be modified at runtime. */
		struct dynamic_type_data
		{
			void clear()
			{
				attrs.clear();
				vtabs.clear();
				bases.clear();
				enums.clear();
				ctors.clear();
				convs.clear();
				cmps.clear();
			}

			/* Attribute constants. */
			attr_table attrs;
			/* Enumeration constants. */
			enum_table enums;
			/* Facet vtables. */
			vtab_table vtabs;
			/* Base types. */
			base_table bases;

			/* Type constructors. */
			std::list<type_ctor> ctors;
			/* Type conversions. */
			conv_table convs;
			/* Comparison functions. */
			cmp_table cmps;
		};
		/* Type data that can be constexpr-initialized. */
		struct constant_type_data
		{
			template<typename T>
			constexpr constant_type_data(std::in_place_type_t<T>) noexcept;

			/* `type_data` initialization function. */
			void (*init_func)(type_data &, database_impl &) = nullptr;
			/* `any` initialization functions. */
			any_funcs_t any_funcs;

			std::string_view name;
			type_flags flags = {};

			std::size_t size = 0;
			std::size_t alignment = 0;

			type_handle remove_pointer = {};
			type_handle remove_extent = {};
			std::size_t extent = 0;
		};

		/* Type data must be over-aligned since `any` uses bottom bits of its pointer for flags. */
		struct alignas(detail::any_flags_max + 1) type_data : constant_type_data, dynamic_type_data
		{
			type_data(const constant_type_data &cdata) : constant_type_data(cdata) {}

			template<typename T>
			REFLEX_COLD static void impl_init(type_data &data, database_impl &db)
			{
				/* Constructors, destructors & conversions are only created for object types. */
				if constexpr (std::is_object_v<T>)
				{
					/* Add default & copy constructors. */
					if constexpr (std::is_default_constructible_v<T>)
						data.ctors.emplace_back(make_type_ctor<T>());
					if constexpr (std::is_copy_constructible_v<T>)
						data.ctors.emplace_back(make_type_ctor<T, std::add_const_t<T> &>());

					/* Add comparisons. */
					if constexpr (std::equality_comparable<T> || std::three_way_comparable<T>)
						data.cmps.emplace(type_name_v<T>, make_type_cmp<T>());

					/* Add enum underlying type overloads. */
					if constexpr (std::is_enum_v<T>)
					{
						data.ctors.emplace_back(make_type_ctor<T, std::underlying_type_t<T>>([](std::underlying_type_t<T> value) { return static_cast<T>(value); }));
						data.convs.emplace(type_name_v<std::underlying_type_t<T>>, make_type_conv<T, std::underlying_type_t<T>>());
						data.cmps.emplace(type_name_v<std::underlying_type_t<T>>, make_type_cmp<std::underlying_type_t<T>>());
					}
				}

				/* Invoke user type initializer. */
				if constexpr (requires(type_factory<T> f) { type_init<T>{}(f); })
				{
					type_factory<T> f{&data, &db};
					type_init<T>{}(f);
				}
			}

			bool walk_bases(database_impl &db, auto &&p) const { return std::any_of(bases.begin(), bases.end(), [&](auto &&e) { return p(e.second.type(db)); }); }

			[[nodiscard]] any *find_attr(std::string_view name) noexcept
			{
				const auto pos = attrs.find(name);
				return pos != attrs.end() ? &pos->second : nullptr;
			}
			[[nodiscard]] const any *find_attr(std::string_view name) const noexcept
			{
				const auto pos = attrs.find(name);
				return pos != attrs.end() ? &pos->second : nullptr;
			}

			[[nodiscard]] const void *find_vtab(std::string_view name, database_impl &db) const
			{
				auto pos = vtabs.find(name);
				if (pos != vtabs.end()) return pos->second;

				const auto pred = [&](const auto *t) { return (pos = t->vtabs.find(name)) != t->vtabs.end(); };
				return walk_bases(db, pred) ? pos->second : nullptr;
			}

			[[nodiscard]] type_base *find_base(std::string_view name, database_impl &db)
			{
				auto pos = bases.find(name);
				if (pos != bases.end()) return &pos->second;

				const auto pred = [&](auto *t) { return (pos = t->bases.find(name)) != t->bases.end(); };
				return walk_bases(db, pred) ? &pos->second : nullptr;
			}
			[[nodiscard]] const type_base *find_base(std::string_view name, database_impl &db) const
			{
				auto pos = bases.find(name);
				if (pos != bases.end()) return &pos->second;

				const auto pred = [&](const auto *t) { return (pos = t->bases.find(name)) != t->bases.end(); };
				return walk_bases(db, pred) ? &pos->second : nullptr;
			}

			[[nodiscard]] any *find_enum(const any &value)
			{
				const auto pos = std::ranges::find_if(enums, [&](auto &&e) { return e.second == value; });
				return pos != enums.end() ? &pos->second : nullptr;
			}
			[[nodiscard]] const any *find_enum(const any &value) const
			{
				const auto pos = std::ranges::find_if(enums, [&](auto &&e) { return e.second == value; });
				return pos != enums.end() ? &pos->second : nullptr;
			}
			[[nodiscard]] any *find_enum(std::string_view name) noexcept
			{
				const auto pos = enums.find(name);
				return pos != enums.end() ? &pos->second : nullptr;
			}
			[[nodiscard]] const any *find_enum(std::string_view name) const noexcept
			{
				const auto pos = enums.find(name);
				return pos != enums.end() ? &pos->second : nullptr;
			}

			[[nodiscard]] type_ctor *find_exact_ctor(const auto &args) noexcept
			{
				for (auto &ctor: ctors)
				{
					if (arg_data::match_exact(ctor.args, args))
						return &ctor;
				}
				return nullptr;
			}
			[[nodiscard]] const type_ctor *find_exact_ctor(const auto &args) const noexcept
			{
				for (auto &ctor: ctors)
				{
					if (arg_data::match_exact(ctor.args, args))
						return &ctor;
				}
				return nullptr;
			}

			[[nodiscard]] type_ctor *find_ctor(const auto &args, database_impl &db)
			{
				type_ctor *candidate = nullptr;
				for (auto &ctor: ctors)
				{
					if (arg_data::match_exact(ctor.args, args))
						return &ctor;
					if (arg_data::match_compatible(ctor.args, args, db))
						candidate = &ctor;
				}
				return candidate;
			}
			[[nodiscard]] const type_ctor *find_ctor(const auto &args, database_impl &db) const
			{
				const type_ctor *candidate = nullptr;
				for (auto &ctor: ctors)
				{
					if (arg_data::match_exact(ctor.args, args))
						return &ctor;
					if (arg_data::match_compatible(ctor.args, args, db))
						candidate = &ctor;
				}
				return candidate;
			}

			[[nodiscard]] type_conv *find_conv(std::string_view name, database_impl &db)
			{
				auto pos = convs.find(name);
				if (pos != convs.end()) return &pos->second;

				const auto pred = [&](auto *t) { return (pos = t->convs.find(name)) != t->convs.end(); };
				return walk_bases(db, pred) ? &pos->second : nullptr;
			}
			[[nodiscard]] const type_conv *find_conv(std::string_view name, database_impl &db) const
			{
				auto pos = convs.find(name);
				if (pos != convs.end()) return &pos->second;

				const auto pred = [&](const auto *t) { return (pos = t->convs.find(name)) != t->convs.end(); };
				return walk_bases(db, pred) ? &pos->second : nullptr;
			}

			[[nodiscard]] type_cmp *find_cmp(std::string_view name) noexcept
			{
				const auto pos = cmps.find(name);
				return pos != cmps.end() ? &pos->second : nullptr;
			}
			[[nodiscard]] const type_cmp *find_cmp(std::string_view name) const noexcept
			{
				const auto pos = cmps.find(name);
				return pos != cmps.end() ? &pos->second : nullptr;
			}

			type_data &init(database_impl &db)
			{
				init_func(*this, db);
				return *this;
			}
			type_data &reset(database_impl &db)
			{
				/* Clear all dynamically-initialized metadata. */
				dynamic_type_data::clear();
				/* Re-initialize the metadata. */
				return init(db);
			}
		};

		bool arg_data::compatible(const any &other, database_impl &db) const
		{
			if (flags < int{other.is_const()} ? is_const : type_flags{})
				return false;

			if (const auto other_name = other.type().name(); type_name != other_name)
			{
				if (const auto this_type = type(db); !this_type->find_base(other_name, db))
					return (flags >= is_const) && this_type->find_conv(other_name, db);
			}
			return true;
		}
		bool arg_data::compatible(const arg_data &other, database_impl &db) const
		{
			if (flags < (other.flags & is_const))
				return false;

			if (const auto other_name = other.type_name; type_name != other_name)
			{
				if (const auto this_type = type(db); !this_type->find_base(other_name, db))
					return (flags >= is_const) && this_type->find_conv(other_name, db);
			}
			return true;
		}

		template<typename T>
		constexpr constant_type_data::constant_type_data(std::in_place_type_t<T>) noexcept
				: init_func(&type_data::impl_init<T>),
				  any_funcs(make_any_funcs<T>()),
				  name(type_name_v<T>)
		{
			if constexpr (std::same_as<T, void>)
				flags |= type_flags::is_void;
			else
			{
				if constexpr (!std::is_empty_v<T>)
				{
					size = sizeof(T);
					alignment = alignof(T);
				}

				if constexpr (std::is_enum_v<T>) flags |= type_flags::is_enum;
				if constexpr (std::is_class_v<T>) flags |= type_flags::is_class;
				if constexpr (std::is_pointer_v<T>) flags |= type_flags::is_pointer;
				if constexpr (std::is_abstract_v<T>) flags |= type_flags::is_abstract;
				if constexpr (std::is_null_pointer_v<T>) flags |= type_flags::is_null;

				if constexpr (std::is_arithmetic_v<T>) flags |= type_flags::is_arithmetic;
				if constexpr (std::signed_integral<T>) flags |= type_flags::is_signed_int;
				if constexpr (std::unsigned_integral<T>) flags |= type_flags::is_unsigned_int;

				remove_pointer = data_factory<std::decay_t<std::remove_pointer_t<T>>>;
				remove_extent = data_factory<std::decay_t<std::remove_extent_t<T>>>;
				extent = std::extent_v<T>;
			}
		}
	}

	constexpr bool argument_info::operator==(const argument_info &other) const noexcept
	{
		return m_data == other.m_data || (m_data && other.m_data && *m_data == *other.m_data);
	}

	template<typename... Args>
	argument_view::argument_view(type_pack_t<Args...>, detail::database_impl *db) noexcept : m_data(detail::make_argument_view<Args...>()), m_db(db) {}

	class argument_view::pointer
	{
		friend class iterator;

		constexpr pointer(argument_info &&info) : m_value(std::forward<argument_info>(info)) {}

	public:
		using element_type = argument_info;

	public:
		pointer() = delete;

		constexpr pointer(const pointer &) noexcept = default;
		constexpr pointer(pointer &&) noexcept = default;
		pointer &operator=(const pointer &) noexcept = default;
		pointer &operator=(pointer &&) noexcept = default;

		[[nodiscard]] constexpr const argument_info &operator*() const noexcept { return m_value; }
		[[nodiscard]] constexpr const argument_info *operator->() const noexcept { return &m_value; }

		[[nodiscard]] constexpr bool operator==(const pointer &) const noexcept = default;

	private:
		argument_info m_value;
	};
	class argument_view::iterator
	{
		friend class argument_view;

		using iter_t = typename std::span<const detail::arg_data>::iterator;

	public:
		using value_type = argument_info;
		using reference = argument_info;
		using pointer = pointer;

		using difference_type = std::ptrdiff_t;
		using iterator_category = std::random_access_iterator_tag;

	private:
		constexpr iterator(iter_t iter, detail::database_impl *db) : m_iter(iter), m_db(db) {}

	public:
		constexpr iterator() noexcept = default;
		constexpr iterator(const iterator &) noexcept = default;
		constexpr iterator(iterator &&) noexcept = default;
		iterator &operator=(const iterator &) noexcept = default;
		iterator &operator=(iterator &&) noexcept = default;

		constexpr iterator operator++(int) noexcept
		{
			auto tmp = *this;
			operator++();
			return tmp;
		}
		constexpr iterator operator--(int) noexcept
		{
			auto tmp = *this;
			operator--();
			return tmp;
		}
		constexpr iterator &operator++() noexcept
		{
			++m_iter;
			return *this;
		}
		constexpr iterator &operator--() noexcept
		{
			--m_iter;
			return *this;
		}

		constexpr iterator &operator+=(difference_type n) noexcept
		{
			m_iter += n;
			return *this;
		}
		constexpr iterator &operator-=(difference_type n) noexcept
		{
			m_iter -= n;
			return *this;
		}

		[[nodiscard]] constexpr iterator operator+(difference_type n) const noexcept { return {m_iter + n, m_db}; }
		[[nodiscard]] constexpr iterator operator-(difference_type n) const noexcept { return {m_iter - n, m_db}; }
		[[nodiscard]] constexpr difference_type operator-(const iterator &other) const noexcept { return m_iter - other.m_iter; }

		[[nodiscard]] constexpr pointer operator->() const noexcept { return get(); }
		[[nodiscard]] constexpr reference operator*() const noexcept { return *get(); }

		[[nodiscard]] constexpr pointer get() const noexcept { return {argument_info{std::to_address(m_iter), m_db}}; }
		[[nodiscard]] constexpr reference operator[](difference_type i) const noexcept { return *pointer{argument_info{&m_iter[i], m_db}}; }

		[[nodiscard]] constexpr bool operator==(const iterator &other) const noexcept { return m_iter == other.m_iter; }
		[[nodiscard]] constexpr auto operator<=>(const iterator &other) const noexcept { return m_iter <=> other.m_iter; }

	private:
		iter_t m_iter = {};
		detail::database_impl *m_db = nullptr;
	};

	constexpr typename argument_view::iterator argument_view::begin() const noexcept { return {m_data.begin(), m_db}; }
	constexpr typename argument_view::iterator argument_view::cbegin() const noexcept { return begin(); }
	constexpr typename argument_view::iterator argument_view::end() const noexcept { return {m_data.end(), m_db}; }
	constexpr typename argument_view::iterator argument_view::cend() const noexcept { return end(); }

	constexpr typename argument_view::value_type argument_view::front() const noexcept { return *begin(); }
	constexpr typename argument_view::value_type argument_view::back() const noexcept { return *(end() - 1); }
	constexpr typename argument_view::value_type argument_view::operator[](size_type i) const noexcept { return begin()[static_cast<difference_type>(i)]; }

	argument_view constructor_info::args() const noexcept { return {m_data->args, m_db}; }
	any constructor_info::invoke(std::span<any> args) const { return m_data->operator()(args); }
	any constructor_info::operator()(std::span<any> args) const { return invoke(args); }
	constexpr bool constructor_info::operator==(const constructor_info &other) const noexcept { return m_data == other.m_data; }

	class constructor_view::pointer
	{
		friend class iterator;

		constexpr pointer(constructor_info &&info) : m_value(std::forward<constructor_info>(info)) {}

	public:
		using element_type = constructor_info;

	public:
		pointer() = delete;

		constexpr pointer(const pointer &) noexcept = default;
		constexpr pointer(pointer &&) noexcept = default;
		pointer &operator=(const pointer &) noexcept = default;
		pointer &operator=(pointer &&) noexcept = default;

		[[nodiscard]] constexpr const constructor_info &operator*() const noexcept { return m_value; }
		[[nodiscard]] constexpr const constructor_info *operator->() const noexcept { return &m_value; }

		[[nodiscard]] constexpr bool operator==(const pointer &) const noexcept = default;

	private:
		constructor_info m_value;
	};
	class constructor_view::iterator
	{
		friend class constructor_view;

		using iter_t = typename std::list<detail::type_ctor>::const_iterator;

	public:
		using value_type = constructor_info;
		using reference = constructor_info;
		using pointer = pointer;

		using difference_type = std::ptrdiff_t;
		using iterator_category = std::bidirectional_iterator_tag;

	private:
		iterator(iter_t iter, detail::database_impl *db) : m_iter(iter), m_db(db) {}

	public:
		constexpr iterator() noexcept = default;
		constexpr iterator(const iterator &) noexcept = default;
		constexpr iterator(iterator &&) noexcept = default;
		iterator &operator=(const iterator &) noexcept = default;
		iterator &operator=(iterator &&) noexcept = default;

		iterator operator++(int) noexcept
		{
			auto tmp = *this;
			operator++();
			return tmp;
		}
		iterator operator--(int) noexcept
		{
			auto tmp = *this;
			operator--();
			return tmp;
		}
		iterator &operator++() noexcept
		{
			++m_iter;
			return *this;
		}
		iterator &operator--() noexcept
		{
			--m_iter;
			return *this;
		}

		[[nodiscard]] constexpr pointer operator->() const noexcept { return get(); }
		[[nodiscard]] constexpr reference operator*() const noexcept { return *get(); }

		[[nodiscard]] constexpr pointer get() const noexcept { return {constructor_info{std::to_address(m_iter), m_db}}; }

		[[nodiscard]] bool operator==(const iterator &other) const noexcept { return m_iter == other.m_iter; }

	private:
		iter_t m_iter = {};
		detail::database_impl *m_db = nullptr;
	};

	typename constructor_view::iterator constructor_view::begin() const noexcept { return {m_view->begin(), m_db}; }
	typename constructor_view::iterator constructor_view::cbegin() const noexcept { return begin(); }
	typename constructor_view::iterator constructor_view::end() const noexcept { return {m_view->end(), m_db}; }
	typename constructor_view::iterator constructor_view::cend() const noexcept { return end(); }

	constexpr std::string_view type_info::name() const noexcept { return valid() ? m_data->name : std::string_view{}; }

	constexpr std::size_t type_info::size() const noexcept { return valid() ? m_data->size : 0; }
	constexpr std::size_t type_info::extent() const noexcept { return valid() ? m_data->extent : 0; }
	constexpr std::size_t type_info::alignment() const noexcept { return valid() ? m_data->alignment : 0; }

	constexpr bool type_info::is_empty() const noexcept { return size() == 0; }
	constexpr bool type_info::is_void() const noexcept { return valid() && (m_data->flags & detail::is_void); }
	constexpr bool type_info::is_nullptr() const noexcept { return valid() && (m_data->flags & detail::is_null); }

	constexpr bool type_info::is_array() const noexcept { return extent() > 0; }
	constexpr bool type_info::is_enum() const noexcept { return valid() && (m_data->flags & detail::is_enum); }
	constexpr bool type_info::is_class() const noexcept { return valid() && (m_data->flags & detail::is_class); }
	constexpr bool type_info::is_abstract() const noexcept { return valid() && (m_data->flags & detail::is_abstract); }

	constexpr bool type_info::is_pointer() const noexcept { return valid() && (m_data->flags & detail::is_pointer); }
	constexpr bool type_info::is_integral() const noexcept { return valid() && (m_data->flags & (detail::is_signed_int | detail::is_unsigned_int)); }
	constexpr bool type_info::is_signed_integral() const noexcept { return valid() && (m_data->flags & detail::is_signed_int); }
	constexpr bool type_info::is_unsigned_integral() const noexcept { return valid() && (m_data->flags & detail::is_unsigned_int); }
	constexpr bool type_info::is_arithmetic() const noexcept { return valid() && (m_data->flags & detail::is_arithmetic); }

	type_info type_info::remove_extent() const noexcept { return valid() && m_data->remove_extent ? type_info{m_data->remove_extent, *m_db} : type_info{}; }
	type_info type_info::remove_pointer() const noexcept { return valid() && m_data->remove_pointer ? type_info{m_data->remove_pointer, *m_db} : type_info{}; }

	constexpr constructor_view type_info::constructors() const noexcept { return valid() ? constructor_view{&m_data->ctors, m_db} : constructor_view{}; }

	template<typename T>
	void any::copy_init(type_info type, T *ptr)
	{
		if constexpr (std::is_const_v<T>)
			(this->*(type_data()->any_funcs.copy_init))(type, ptr, nullptr);
		else
			(this->*(type_data()->any_funcs.copy_init))(type, nullptr, ptr);
	}
	template<typename T>
	void any::copy_assign(type_info type, T *ptr)
	{
		if constexpr (std::is_const_v<T>)
			(this->*(type_data()->any_funcs.copy_assign))(type, ptr, nullptr);
		else
			(this->*(type_data()->any_funcs.copy_assign))(type, nullptr, ptr);
	}
}