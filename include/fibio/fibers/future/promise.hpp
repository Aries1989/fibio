//
//  promise.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-29.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_fibers_future_promise_hpp
#define fibio_fibers_future_promise_hpp

#include <memory>

#include <boost/config.hpp>
#include <boost/move/move.hpp>
#include <boost/thread/detail/memory.hpp> // boost::allocator_arg_t
#include <boost/throw_exception.hpp>
#include <boost/utility.hpp>

#include <fibio/fibers/exceptions.hpp>
#include <fibio/fibers/future/detail/shared_state.hpp>
#include <fibio/fibers/future/detail/shared_state_object.hpp>
#include <fibio/fibers/future/future.hpp>

namespace fibio { namespace fibers {
    template< typename R >
    class promise : private boost::noncopyable
    {
    private:
        typedef typename detail::shared_state< R >::ptr_t   ptr_t;
        
        struct dummy
        { void nonnull() {} };
        
        typedef void ( dummy::*safe_bool)();
        
        bool            obtained_;
        ptr_t           future_;
        
        BOOST_MOVABLE_BUT_NOT_COPYABLE( promise);
        
    public:
        promise() :
        obtained_( false),
        future_()
        {
            // TODO: constructs the promise with an empty shared state
            //       the shared state is allocated using alloc
            //       alloc must meet the requirements of Allocator
            typedef detail::shared_state_object<
            R, std::allocator< promise< R > >
            >                                               object_t;
            std::allocator< promise< R > > alloc;
            typename object_t::allocator_t a( alloc);
            future_ = ptr_t(
                            // placement new
                            ::new( a.allocate( 1) ) object_t( a) );
        }
        
        template< typename Allocator >
        promise( boost::allocator_arg_t, Allocator alloc) :
        obtained_( false),
        future_()
        {
            // TODO: constructs the promise with an empty shared state
            //       the shared state is allocated using alloc
            //       alloc must meet the requirements of Allocator
            typedef detail::shared_state_object< R, Allocator >  object_t;
            typename object_t::allocator_t a( alloc);
            future_ = ptr_t(
                            // placement new
                            ::new( a.allocate( 1) ) object_t( a) );
        }
        
        ~promise()
        {
            //TODO: abandon ownership if any
            if ( future_)
                future_->owner_destroyed();
        }
        
        promise( promise && other) BOOST_NOEXCEPT :
        obtained_( false),
        future_()
        {
            //TODO: take over ownership
            //      other is valid before but in
            //      undefined state afterwards
            swap( other);
        }
        
        promise & operator=( promise && other) BOOST_NOEXCEPT
        {
            //TODO: take over ownership
            //      other is valid before but in
            //      undefined state afterwards
            promise tmp( boost::move( other) );
            swap( tmp);
            return * this;
        }
        
        void swap( promise & other) BOOST_NOEXCEPT
        {
            //TODO: exchange the shared states of two promises
            std::swap( obtained_, other.obtained_);
            future_.swap( other.future_);
        }
        
        operator safe_bool() const BOOST_NOEXCEPT
        { return 0 != future_.get() ? & dummy::nonnull : 0; }
        
        bool operator!() const BOOST_NOEXCEPT
        { return 0 == future_.get(); }
        
        future< R > get_future()
        {
            //TODO: returns a future object associated with the same shared state
            //      exception is thrown if *this has no shared state or get_future
            //      has already been called.
            if ( obtained_)
                BOOST_THROW_EXCEPTION(future_already_retrieved());
            if ( ! future_)
                BOOST_THROW_EXCEPTION(promise_uninitialized());
            obtained_ = true;
            return future< R >( future_);
        }
        
        void set_value( R const& value)
        {
            //TODO: store the value into the shared state and make the state ready
            //      the operation is atomic, i.e. it behaves as though they acquire a single mutex
            //      associated with the promise object while updating the promise object
            //      an exception is thrown if there is no shared state or the shared state already
            //      stores a value or exception
            if ( ! future_)
                BOOST_THROW_EXCEPTION(promise_uninitialized());
            future_->set_value( value);
        }
        
        void set_value( R && value)
        {
            //TODO: store the value into the shared state and make the state ready
            //      rhe operation is atomic, i.e. it behaves as though they acquire a single mutex
            //      associated with the promise object while updating the promise object
            //      an exception is thrown if there is no shared state or the shared state already
            //      stores a value or exception
            if ( ! future_)
                BOOST_THROW_EXCEPTION(promise_uninitialized());
            future_->set_value( boost::move( value) );
        }
        
        void set_exception( std::exception_ptr p)
        {
            //TODO: store the exception pointer p into the shared state and make the state ready
            //      the operation is atomic, i.e. it behaves as though they acquire a single mutex
            //      associated with the promise object while updating the promise object
            //      an exception is thrown if there is no shared state or the shared state already
            //      stores a value or exception
            if ( ! future_)
                BOOST_THROW_EXCEPTION(promise_uninitialized());
            future_->set_exception( p);
        }
    };
    
    template< typename R >
    class promise< R & > : private boost::noncopyable
    {
    private:
        typedef typename detail::shared_state< R & >::ptr_t   ptr_t;
        
        struct dummy
        { void nonnull() {} };
        
        typedef void ( dummy::*safe_bool)();
        
        bool            obtained_;
        ptr_t           future_;
        
        BOOST_MOVABLE_BUT_NOT_COPYABLE( promise);
        
    public:
        promise() :
        obtained_( false),
        future_()
        {
            // TODO: constructs the promise with an empty shared state
            //       the shared state is allocated using alloc
            //       alloc must meet the requirements of Allocator
            typedef detail::shared_state_object<
            R &, std::allocator< promise< R & > >
            >                                               object_t;
            std::allocator< promise< R > > alloc;
            typename object_t::allocator_t a( alloc);
            future_ = ptr_t(
                            // placement new
                            ::new( a.allocate( 1) ) object_t( a) );
        }
        
        template< typename Allocator >
        promise( boost::allocator_arg_t, Allocator alloc) :
        obtained_( false),
        future_()
        {
            // TODO: constructs the promise with an empty shared state
            //       the shared state is allocated using alloc
            //       alloc must meet the requirements of Allocator
            typedef detail::shared_state_object< R &, Allocator >  object_t;
            typename object_t::allocator_t a( alloc);
            future_ = ptr_t(
                            // placement new
                            ::new( a.allocate( 1) ) object_t( a) );
        }
        
        ~promise()
        {
            //TODO: abandon ownership if any
            if ( future_)
                future_->owner_destroyed();
        }
        
        promise( promise && other) BOOST_NOEXCEPT :
        obtained_( false),
        future_()
        {
            //TODO: take over ownership
            //      other is valid before but in
            //      undefined state afterwards
            swap( other);
        }
        
        promise & operator=( promise && other) BOOST_NOEXCEPT
        {
            //TODO: take over ownership
            //      other is valid before but in
            //      undefined state afterwards
            promise tmp( boost::move( other) );
            swap( tmp);
            return * this;
        }
        
        void swap( promise & other) BOOST_NOEXCEPT
        {
            //TODO: exchange the shared states of two promises
            std::swap( obtained_, other.obtained_);
            future_.swap( other.future_);
        }
        
        operator safe_bool() const BOOST_NOEXCEPT
        { return 0 != future_.get() ? & dummy::nonnull : 0; }
        
        bool operator!() const BOOST_NOEXCEPT
        { return 0 == future_.get(); }
        
        future< R & > get_future()
        {
            //TODO: returns a future object associated with the same shared state
            //      exception is thrown if *this has no shared state or get_future
            //      has already been called.
            if ( obtained_)
                BOOST_THROW_EXCEPTION(future_already_retrieved());
            if ( ! future_)
                BOOST_THROW_EXCEPTION(promise_uninitialized());
            obtained_ = true;
            return future< R & >( future_);
        }
        
        void set_value( R & value)
        {
            //TODO: store the value into the shared state and make the state ready
            //      the operation is atomic, i.e. it behaves as though they acquire a single mutex
            //      associated with the promise object while updating the promise object
            //      an exception is thrown if there is no shared state or the shared state already
            //      stores a value or exception
            if ( ! future_)
                BOOST_THROW_EXCEPTION(promise_uninitialized());
            future_->set_value( value);
        }
        
        void set_exception( std::exception_ptr p)
        {
            //TODO: store the exception pointer p into the shared state and make the state ready
            //      the operation is atomic, i.e. it behaves as though they acquire a single mutex
            //      associated with the promise object while updating the promise object
            //      an exception is thrown if there is no shared state or the shared state already
            //      stores a value or exception
            if ( ! future_)
                BOOST_THROW_EXCEPTION(promise_uninitialized());
            future_->set_exception( p);
        }
    };
    
    template<>
    class promise< void > : private boost::noncopyable
    {
    private:
        typedef detail::shared_state< void >::ptr_t   ptr_t;
        
        struct dummy
        { void nonnull() {} };
        
        typedef void ( dummy::*safe_bool)();
        
        bool            obtained_;
        ptr_t           future_;
        
        BOOST_MOVABLE_BUT_NOT_COPYABLE( promise);
        
    public:
        promise() :
        obtained_( false),
        future_()
        {
            // TODO: constructs the promise with an empty shared state
            //       the shared state is allocated using alloc
            //       alloc must meet the requirements of Allocator
            typedef detail::shared_state_object<
            void, std::allocator< promise< void > >
            >                                               object_t;
            std::allocator< promise< void > > alloc;
            object_t::allocator_t a( alloc);
            future_ = ptr_t(
                            // placement new
                            ::new( a.allocate( 1) ) object_t( a) );
        }
        
        template< typename Allocator >
        promise( boost::allocator_arg_t, Allocator alloc) :
        obtained_( false),
        future_()
        {
            // TODO: constructs the promise with an empty shared state
            //       the shared state is allocated using alloc
            //       alloc must meet the requirements of Allocator
            typedef detail::shared_state_object< void, Allocator >  object_t;
#if BOOST_MSVC
            object_t::allocator_t a( alloc);
#else
            typename object_t::allocator_t a( alloc);
#endif
            future_ = ptr_t(
                            // placement new
                            ::new( a.allocate( 1) ) object_t( a) );
        }
        
        ~promise()
        {
            //TODO: abandon ownership if any
            if ( future_)
                future_->owner_destroyed();
        }
        
        promise( promise && other) BOOST_NOEXCEPT :
        obtained_( false),
        future_()
        {
            //TODO: take over ownership
            //      other is valid before but in
            //      undefined state afterwards
            swap( other);
        }
        
        promise & operator=( promise && other) BOOST_NOEXCEPT
        {
            //TODO: take over ownership
            //      other is valid before but in
            //      undefined state afterwards
            promise tmp( boost::move( other) );
            swap( tmp);
            return * this;
        }
        
        void swap( promise & other) BOOST_NOEXCEPT
        {
            //TODO: exchange the shared states of two promises
            std::swap( obtained_, other.obtained_);
            future_.swap( other.future_);
        }
        
        operator safe_bool() const BOOST_NOEXCEPT
        { return 0 != future_.get() ? & dummy::nonnull : 0; }
        
        bool operator!() const BOOST_NOEXCEPT
        { return 0 == future_.get(); }
        
        future< void > get_future()
        {
            //TODO: returns a future object associated with the same shared state
            //      exception is thrown if *this has no shared state or get_future
            //      has already been called. 
            if ( obtained_)
                BOOST_THROW_EXCEPTION(future_already_retrieved());
            if ( ! future_)
                BOOST_THROW_EXCEPTION(promise_uninitialized());
            obtained_ = true;
            return future< void >( future_);
        }
        
        void set_value()
        {
            //TODO: store the value into the shared state and make the state ready
            //      the operation is atomic, i.e. it behaves as though they acquire a single mutex
            //      associated with the promise object while updating the promise object
            //      an exception is thrown if there is no shared state or the shared state already
            //      stores a value or exception
            if ( ! future_)
                BOOST_THROW_EXCEPTION(promise_uninitialized());
            future_->set_value();
        }
        
        void set_exception( std::exception_ptr p)
        {
            //TODO: store the exception pointer p into the shared state and make the state ready
            //      the operation is atomic, i.e. it behaves as though they acquire a single mutex
            //      associated with the promise object while updating the promise object
            //      an exception is thrown if there is no shared state or the shared state already
            //      stores a value or exception
            if ( ! future_)
                BOOST_THROW_EXCEPTION(promise_uninitialized());
            future_->set_exception( p);
        }
    };
    
    template< typename R >
    void swap( promise< R > & l, promise< R > & r)
    { l.swap( r); }

    
    template <typename T>
    future<typename std::decay<T>::type> make_ready_future(T&& value) {
        typedef typename std::decay<T>::type value_type;
        promise<value_type> p;
        p.set_value(std::forward<T>(value));
        return p.get_future();
    }
    
    inline future<void> make_ready_future() {
        promise<void> p;
        p.set_value();
        return p.get_future();
    }
}}

namespace fibio {
    using fibers::promise;
    using fibers::make_ready_future;
}

#endif
