#ifndef _UTILITIES_H
#define _UTILITIES_H

#include "standard.h"
#include <typeinfo>
#include <vector>
#include <map>
#include <string>
#include <stdexcept>
#include <algorithm>
#include <utility>
#include <iterator>

namespace Utilities
{
	// this is not for the user
	namespace detail
	{
		class type_index
		{
		public:
			type_index(const std::type_info& type) : t(type) {}
			inline bool operator<(const type_index& rhs) const { return t.before(rhs.t); }
			
		private:
			const std::type_info& t;
		};
	}
	
	//typedef double PosType;
	
	template <class T>
	void Matrix(T **matrix, long rows, long cols){
		matrix = new T*[rows];
		matrix[0] = new T[rows*cols];
		for (long i = 1; i < rows; ++i)
			matrix[i] = matrix[0] + i * cols;
	}
	
	template <class T>
	void free_Matrix(T **matrix, long rows, long){
		if (rows) delete[] matrix[0];
		delete []matrix;
	}
	
	PosType **PosTypeMatrix(long rows, long cols);
	void free_PosTypeMatrix(PosType **matrix, long rows, long cols);
	
	/** \brief A container that can hold mixed objects all derived from
	 * a base class and retains the ability to access derived class functions/members.
	 *
	 * The indexing operator is overridden to return a reference to the base class and
	 * a templated index<>() function provides a reference to elements with derived class
	 * type information.
	 *
	 *
	 *<pre>
	 * example:
	 * InputParams params(paramfile);
     * params.put("gauss_r2",1,"non");
 	 * SourceGaussian source1(params);
	 * SourceUniform source2(params);
     *
	 * Utilities::MixedVector<Source> mvector;
     *
	 * mvector.push_back(source1);
	 * mvector.push_back(source1);
	 * mvector.push_back(source2);
	 * mvector.push_back(source2);
	 * cout << "Number of Uniform Sources " << mvector.size<SourceUniform>() << "   Number of Gausssian Sources "
	 *		<< mvector.size<SourceGaussian>() << endl;
     *
     * // change derived class attribute
	 * mvector.at<SourceGaussian>(0).source_gauss_r2 = 0.5;
	 * cout << "A base class attribute " << mvector[2].getTotalFlux()
	 *		<< " A derived class attribute  "
	 *		<< mvector.at<SourceGaussian>(0).source_gauss_r2
	 *		<< "  " << mvector.at<SourceGaussian>(1).source_gauss_r2
	 *		<< endl;
	 *
	 *</pre>
	 */
	template<typename BaseT>
	class MixedVector
	{
	private: /* forward declarations */
		class base_holder;
		
	public: /* iterators */
		template<typename ValueT = BaseT> // TODO: needs to check subclass type
		class iterator
		{
		private:
			typedef typename std::vector<base_holder*>::iterator base_iterator;
			base_iterator it;
			
		public:
			typedef ValueT value_type;
			typedef ValueT* pointer;
			typedef ValueT& reference;
			typedef typename base_iterator::difference_type difference_type;
			typedef typename base_iterator::iterator_category iterator_category;
			
			iterator() {}
			iterator(base_iterator i) : it(i) {}
			iterator(const iterator& other) : it(other.it) {}
			
			iterator& operator=(const iterator& rhs) { it = rhs.it; return *this; }
			
			reference operator*() { return (reference)(*it)->ref(); }
			const reference operator*() const { return (const reference)(*it)->ref(); }
			
			iterator& operator++() { ++it; return *this; }
			iterator operator++(int) { iterator tmp(*this); ++it; return tmp; }
			iterator& operator--() { --it; return *this; }
			iterator operator--(int) { iterator tmp(*this); --it; return tmp; }
			
			bool operator==(const iterator& rhs) const { return (it == rhs.it); }
			bool operator!=(const iterator& rhs) const { return (it != rhs.it); }
			
			iterator& operator+=(difference_type n) { it += n; return *this; }
			iterator& operator-=(difference_type n) { it -= n; return *this; }
			
			reference operator[](difference_type n) { return (ValueT&)it[n]->ref(); }
			const reference operator[](difference_type n) const { return (const ValueT&)it[n]->ref(); }
			
			friend iterator operator+(const iterator& i, difference_type n) { return iterator(i.it + n); }
			friend iterator operator+(difference_type n, const iterator& i) { return iterator(i.it + n); }
			friend iterator operator-(const iterator& i, difference_type n) { return iterator(i.it - n); }
			friend iterator operator-(difference_type n, const iterator& i) { return iterator(i.it - n); }
			
			friend difference_type operator-(const iterator& b, const iterator& a) { return (b.it - a.it); }
			
			friend bool operator<(const iterator&a, const iterator& b) { return (a.it < b.it); }
			friend bool operator>(const iterator&a, const iterator& b) { return (a.it > b.it); }
			friend bool operator<=(const iterator&a, const iterator& b) { return (a.it <= b.it); }
			friend bool operator>=(const iterator&a, const iterator& b) { return (a.it >= b.it); }
		};
		
	public:
		/// default constructor
		MixedVector()
		{
		}
		
		/// copy the vector and all contained elements
		MixedVector(const MixedVector<BaseT>& other)
		{
			// needs to clone all the contained base_holder items
			for(typename std::vector<base_holder*>::const_iterator it = other.items.begin(); it != other.items.end(); ++it)
			{
				// clone the object
				base_holder* hold = (*it)->clone();
				
				// add to own items
				items.push_back(hold);
				
				// add to own typemap
				tmap[hold->type()].push_back(hold);
			}
		}
		
		/// destroy the vector and all contained items
		~MixedVector()
		{
			clear();
		}
		
		/// swap two MixedVectors
		friend void swap(MixedVector<BaseT>& a, MixedVector<BaseT>& b)
		{
			using std::swap;
			
			swap(a.items, b.items);
			swap(a.tmap, b.tmap);
		}
		
		/// assign one MixedVector to another
		MixedVector<BaseT>& operator=(MixedVector<BaseT> rhs)
		{
			// copy-and-swap idiom
			swap(*this, rhs);
			return *this;
		}
		
		/// add an object of type SubclassT to the vector
		template<typename SubclassT>
		void push_back(const SubclassT& obj)
		{
			// make sure this is a subclass of BaseT
			check_subclass(obj);
			
			// copy the object
			base_holder* hold = new holder<SubclassT>(obj);
			
			// add the copy of the object to the list of items
			items.push_back(hold);
			
			// add the copy to type map
			tmap[typeid(SubclassT)].push_back(hold);
		}
		
		/// pop element from back of vector
		void pop_back()
		{
			// get very last element
			base_holder* back = items.back();
			
			// remove from the vector in the type map
			tmap[back->type()].pop_back();
			
			// remove from items
			items.pop_back();
			
			// delete from memory
			delete back;
		}
		
		/// erase the last element of a specific type
		template<typename SubclassT>
		void pop_back()
		{
			// search for vector belonging to SubclassT
			typename tmap_t::iterator found = tmap.find(typeid(SubclassT));
			if(found == tmap.end())
				return;
			
			// pop from vector for type
			found->second.pop_back();
			
			// go through list of all items from back, until item of correct type is found
			for(typename std::vector<base_holder*>::reverse_iterator it = items.rbegin(); it != items.rend(); ++it)
			{
				if((*it)->type() == typeid(SubclassT))
				{
					// remove the item
					delete (*it);
					items.erase(it);
					return;
				}
			}
		}
		
		/// clear all elements
		void clear()
		{
			for(std::size_t i = 0, n = items.size(); i < n; ++i)
				delete items[i];
			items.clear();
			tmap.clear();
		}
		
		/// clear all elements of a given type
		template<typename SubclassT>
		void clear()
		{
			// remove vector for subclass
			tmap.erase(typeid(SubclassT));
			
			// erase items matching type
			items.erase(std::remove_if(items.begin(), items.end(), is_type<SubclassT>), items.end());
		}
		
		/// Checks if element i is of the derived type SubclassT
		template<typename SubclassT>
		bool CheckType(std::size_t i)
		{
			return is_type<SubclassT>(items[i]);
		}
		
		/// Indexing operator for all elements.
		BaseT& operator[](std::size_t i)
		{
			return items[i]->ref();
		}
		
		/// Indexing operator for all elements (const).
		const BaseT& operator[](std::size_t i) const
		{
			return items[i]->ref();
		}
		
		/// indexed access
		BaseT& get(std::size_t i)
		{
			return items[i]->ref();
		}
		
		/// indexed access (const)
		const BaseT& get(std::size_t i) const
		{
			return items[i]->ref();
		}
		
		/// indexed access for type SubclassT
		template<typename SubclassT>
		SubclassT& get(std::size_t i)
		{
			typename tmap_t::iterator found = tmap.find(typeid(SubclassT));
			if(found == tmap.end())
				throw std::out_of_range(std::string() + "type " + typeid(SubclassT).name() + " not in vector");
			return (SubclassT&)found->second[i]->ref();
		}
		
		/// indexed access for type SubclassT (const)
		template<typename SubclassT>
		const SubclassT& get(std::size_t i) const
		{
			typename tmap_t::const_iterator found = tmap.find(typeid(SubclassT));
			if(found == tmap.end())
				throw std::out_of_range(std::string() + "type " + typeid(SubclassT).name() + " not in vector");
			return (const SubclassT&)found->second[i]->ref();
		}
		
		/// indexed access with bounds checking
		BaseT& at(std::size_t i)
		{
			return items.at(i)->ref();
		}
		
		/// indexed access with bounds checking (const)
		const BaseT& at(std::size_t i) const
		{
			return items.at(i)->ref();
		}
		
		/** \brief Templated indexing operator for elements of a specific derived class.
		 * The index runs 0 ... size<SubclassT>() - 1
		 * Does bounds checking.
		 */
		template<typename SubclassT>
		SubclassT& at(std::size_t i)
		{
			typename tmap_t::iterator found = tmap.find(typeid(SubclassT));
			if(found == tmap.end())
				throw std::out_of_range(std::string() + "type " + typeid(SubclassT).name() + " not in vector");
			return (SubclassT&)found->second.at(i)->ref();
		}
		
		/// Templated indexing operator for elements of a specific derived class (const).
		template<typename SubclassT>
		const SubclassT& at(std::size_t i) const
		{
			typename tmap_t::const_iterator found = tmap.find(typeid(SubclassT));
			if(found == tmap.end())
				throw std::out_of_range(std::string() + "type " + typeid(SubclassT).name() + " not in vector");
			return (const SubclassT&)found->second.at(i)->ref();
		}
		
		/// number of all elements
		std::size_t size() const
		{
			return items.size();
		}
		
		/// number of elements of a specific type
		template<typename SubclassT>
		std::size_t size() const
		{
			typename tmap_t::const_iterator found = tmap.find(typeid(SubclassT));
			if(found == tmap.end())
				return 0;
			return found->second.size();
		}
		
		/// check if vector of items is empty
		bool empty() const
		{
			return items.empty();
		}
		
		/// check if vector of items of type SubclassT is empty
		template<typename SubclassT>
		bool empty() const
		{
			typename tmap_t::const_iterator found = tmap.find(typeid(SubclassT));
			if(found == tmap.end())
				return true;
			
			return found->second.empty();
		}
		
		/// get iterator to first of all items
		iterator<> begin()
		{
			return iterator<>(items.begin());
		}
		
		/// get iterator to last of all items
		iterator<> end()
		{
			return iterator<>(items.end());
		}
		
		/// get iterator to first of items of type SubclassT
		template<typename SubclassT>
		iterator<SubclassT> begin()
		{
			typename tmap_t::iterator found = tmap.find(typeid(SubclassT));
			if(found == tmap.end())
				return iterator<SubclassT>(items.end());
			return iterator<SubclassT>(found->second.begin());
		}
		
		/// get iterator to last of items of type SubclassT
		template<typename SubclassT>
		iterator<SubclassT> end()
		{
			typename tmap_t::iterator found = tmap.find(typeid(SubclassT));
			if(found == tmap.end())
				return iterator<SubclassT>(items.end());
			return iterator<SubclassT>(found->second.end());
		}
		
	private:
		class base_holder
		{
		public:
			virtual ~base_holder() {}
			
			virtual base_holder* clone() const = 0;
			virtual const std::type_info& type() const = 0;
			virtual BaseT& ref() = 0;
			virtual const BaseT& ref() const = 0;
		};
		
		template<typename T>
		class holder : public base_holder
		{
		public:
			holder(const T& held) { ptr = new T(held); }
			~holder() { delete ptr; }
			
			holder* clone() const { return new holder<T>(*ptr); }
			const std::type_info& type() const { return typeid(T); }
			T& ref() { return *ptr; }
			const T& ref() const { return *ptr; }
			
		private:
			T* ptr;
		};
		
		typedef std::vector<base_holder*> base_container;
		
		inline static void check_subclass(const BaseT&) {}
		
		template<typename SubclassT>
		inline static bool is_type(const base_holder* obj) { return (obj->type() == typeid(SubclassT)); }
		
		base_container items;
		
		typedef std::map<detail::type_index, base_container> tmap_t;
		tmap_t tmap;
	};
	
	/// A MixedVector for pointers.
	template<typename BaseT>
	class MixedVector<BaseT*>
	{
	public: /* iterators */
		template<typename ValueT = BaseT> // TODO: needs to check subclass type
		class iterator
		{
		private:
			typedef typename std::vector<BaseT*>::iterator base_iterator;
			base_iterator it;
			
		public:
			typedef ValueT value_type;
			typedef ValueT* pointer;
			typedef ValueT& reference;
			typedef typename base_iterator::difference_type difference_type;
			typedef typename base_iterator::iterator_category iterator_category;
			
			iterator() {}
			iterator(base_iterator i) : it(i) {}
			iterator(const iterator& other) : it(other.it) {}
			
			iterator& operator=(const iterator& rhs) { it = rhs.it; return *this; }
			
			pointer operator*() { return (pointer)*it; }
			const pointer operator*() const { return (const pointer)*it; }
			
			iterator& operator++() { ++it; return *this; }
			iterator operator++(int) { iterator tmp(*this); ++it; return tmp; }
			iterator& operator--() { --it; return *this; }
			iterator operator--(int) { iterator tmp(*this); --it; return tmp; }
			
			bool operator==(const iterator& rhs) const { return (it == rhs.it); }
			bool operator!=(const iterator& rhs) const { return (it != rhs.it); }
			
			iterator& operator+=(difference_type n) { it += n; return *this; }
			iterator& operator-=(difference_type n) { it -= n; return *this; }
			
			pointer operator[](difference_type n) { return (pointer)it[n]; }
			const pointer operator[](difference_type n) const { return (const pointer)it[n]; }
			
			friend iterator operator+(const iterator& i, difference_type n) { return iterator(i.it + n); }
			friend iterator operator+(difference_type n, const iterator& i) { return iterator(i.it + n); }
			friend iterator operator-(const iterator& i, difference_type n) { return iterator(i.it - n); }
			friend iterator operator-(difference_type n, const iterator& i) { return iterator(i.it - n); }
			
			friend difference_type operator-(const iterator& b, const iterator& a) { return (b.it - a.it); }
			
			friend bool operator<(const iterator&a, const iterator& b) { return (a.it < b.it); }
			friend bool operator>(const iterator&a, const iterator& b) { return (a.it > b.it); }
			friend bool operator<=(const iterator&a, const iterator& b) { return (a.it <= b.it); }
			friend bool operator>=(const iterator&a, const iterator& b) { return (a.it >= b.it); }
		};
		
	public:
		/// default constructor
		MixedVector()
		{
		}
		
		/// copy constructor
		MixedVector(const MixedVector<BaseT*>& other)
		: items(other.items), tmap(other.tmap)
		{
		}
		
		/// destroy the vector and all contained items
		~MixedVector()
		{
			clear();
		}
		
		/// swap two MixedVectors
		friend void swap(MixedVector<BaseT*>& a, MixedVector<BaseT*>& b)
		{
			using std::swap;
			
			swap(a.items, b.items);
			swap(a.tmap, b.tmap);
		}
		
		/// assign one MixedVector to another
		MixedVector<BaseT*>& operator=(MixedVector<BaseT*> rhs)
		{
			// copy-and-swap idiom
			swap(*this, rhs);
			return *this;
		}
		
		/// add an object of type SubclassT to the vector
		void push_back(BaseT* obj)
		{
			// make sure this is a subclass of BaseT
			check_subclass(obj);
			
			// add the copy of the object to the list of items
			items.push_back(obj);
			
			// add the copy to type map
			tmap[typeid(*obj)].push_back(obj);
		}
		
		/// pop element from back of vector
		void pop_back()
		{
			// get very last element
			BaseT* back = items.back();
			
			// remove from the vector in the type map
			tmap[typeid(*back)].pop_back();
			
			// remove from items
			items.pop_back();
			
			// delete from memory
			delete back;
		}
		
		/// erase the last element of a specific type
		template<typename SubclassT>
		void pop_back()
		{
			// search for vector belonging to SubclassT
			typename tmap_t::iterator found = tmap.find(typeid(SubclassT));
			if(found == tmap.end())
				return;
			
			// pop from vector for type
			found->second.pop_back();
			
			// go through list of all items from back, until item of correct type is found
			for(typename std::vector<BaseT*>::reverse_iterator it = items.rbegin(); it != items.rend(); ++it)
			{
				if(typeid(**it) == typeid(SubclassT))
				{
					// remove the item
					delete(*it);
					items.erase(it);
					return;
				}
			}
		}
		
		/// clear all elements
		void clear()
		{
			while(!empty()) pop_back();
		}
		
		/// clear all elements of a given type
		template<typename SubclassT>
		void clear()
		{
			// remove vector for subclass
			tmap.erase(typeid(SubclassT));
			
			// erase items matching type
			items.erase(std::remove_if(items.begin(), items.end(), is_type<SubclassT>), items.end());
		}
		
		/// direct access to underlying array
		BaseT** data()
		{
			return items.data();
		}
		
		/// direct access to underlying array (const)
		const BaseT** data() const
		{
			return items.data();
		}
		
		/// Checks if element i is of the derived type SubclassT
		template<typename SubclassT>
		bool CheckType(std::size_t i)
		{
			return is_type<SubclassT>(items[i]);
		}
		
		/// Indexing operator for all elements in form of a reference to the base class.
		BaseT* operator[](std::size_t i) const
		{
			return items[i];
		}
		
		/// indexed access
		BaseT* get(std::size_t i)
		{
			return items[i];
		}
		
		/// indexed access (const)
		const BaseT* get(std::size_t i) const
		{
			return items[i];
		}
		
		/// indexed access for type SubclassT
		template<typename SubclassT>
		SubclassT* get(std::size_t i)
		{
			typename tmap_t::iterator found = tmap.find(typeid(SubclassT));
			if(found == tmap.end())
				throw std::out_of_range(std::string() + "type " + typeid(SubclassT).name() + " not in vector");
			return (SubclassT*)found->second[i];
		}
		
		/// indexed access for type SubclassT (const)
		template<typename SubclassT>
		const SubclassT* get(std::size_t i) const
		{
			typename tmap_t::const_iterator found = tmap.find(typeid(SubclassT));
			if(found == tmap.end())
				throw std::out_of_range(std::string() + "type " + typeid(SubclassT).name() + " not in vector");
			return (const SubclassT*)found->second[i];
		}
		
		/// indexed access with bounds checking
		BaseT* at(std::size_t i)
		{
			return items.at(i);
		}
		
		/// indexed access with bounds checking (const)
		const BaseT* at(std::size_t i) const
		{
			return items.at(i);
		}
		
		/** \brief Templated indexing operator for elements of a specific derived class.
		 * The index runs 0 ... size<SubclassT>() - 1
		 * Does bounds checking.
		 */
		template<typename SubclassT>
		SubclassT* at(std::size_t i)
		{
			typename tmap_t::iterator found = tmap.find(typeid(SubclassT));
			if(found == tmap.end())
				throw std::out_of_range(std::string() + "type " + typeid(SubclassT).name() + " not in vector");
			return (SubclassT*)found->second.at(i);
		}
		
		/// Templated indexing operator for elements of a specific derived class (const).
		template<typename SubclassT>
		const SubclassT* at(std::size_t i) const
		{
			typename tmap_t::const_iterator found = tmap.find(typeid(SubclassT));
			if(found == tmap.end())
				throw std::out_of_range(std::string() + "type " + typeid(SubclassT).name() + " not in vector");
			return (const SubclassT*)found->second.at(i);
		}
		
		/// number of all elements
		std::size_t size() const
		{
			return items.size();
		}
		
		/// number of elements of a specific type
		template<typename SubclassT>
		std::size_t size() const
		{
			typename tmap_t::const_iterator found = tmap.find(typeid(SubclassT));
			if(found == tmap.end())
				return 0;
			return found->second.size();
		}
		
		/// check if vector of items is empty
		bool empty() const
		{
			return items.empty();
		}
		
		/// check if vector of items of type SubclassT is empty
		template<typename SubclassT>
		bool empty() const
		{
			typename tmap_t::const_iterator found = tmap.find(typeid(SubclassT));
			if(found == tmap.end())
				return true;
			
			return found->second.empty();
		}
		
		/// get iterator to first of all items
		iterator<> begin()
		{
			return iterator<>(items.begin());
		}
		
		/// get iterator to last of all items
		iterator<> end()
		{
			return iterator<>(items.end());
		}
		
		/// get iterator to first of items of type SubclassT
		template<typename SubclassT>
		iterator<SubclassT> begin()
		{
			typename tmap_t::iterator found = tmap.find(typeid(SubclassT));
			if(found == tmap.end())
				return iterator<SubclassT>(items.end());
			return iterator<SubclassT>(found->second.begin());
		}
		
		/// get iterator to last of items of type SubclassT
		template<typename SubclassT>
		iterator<SubclassT> end()
		{
			typename tmap_t::iterator found = tmap.find(typeid(SubclassT));
			if(found == tmap.end())
				return iterator<SubclassT>(items.end());
			return iterator<SubclassT>(found->second.end());
		}
		
	private:
		inline void check_subclass(const BaseT*) {}
		
		template<typename SubclassT>
		inline static bool is_type(const BaseT* obj) { return (typeid(*obj) == typeid(SubclassT)); }
		
		typedef std::vector<BaseT*> base_container;
		
		base_container items;
		
		typedef std::map<detail::type_index, base_container> tmap_t;
		tmap_t tmap;
	};
	
	template<class BaseT>
	std::size_t lower_bound(std::vector<BaseT*>& items, double target){
		std::size_t ju,jm,jl;
		
		jl=0;
		ju=items.size()-1;
		while (ju-jl > 1) {
			jm=(ju+jl) >> 1;
			if(items[jm]->compare(target))
				jl=jm;
			else
				ju=jm;
		}
		return jl;
	}
	
	template<typename Container>
	void delete_container(Container& c) { while(!c.empty()) delete c.back(), c.pop_back(); }
}

#endif
