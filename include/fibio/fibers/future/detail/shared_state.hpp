//
//  shared_state.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-29.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_fibers_future_detail_shared_state_hpp
#define fibio_fibers_future_detail_shared_state_hpp

#include <chrono>
#include <boost/assert.hpp>
#include <boost/atomic.hpp>
#include <boost/config.hpp>
#include <boost/exception_ptr.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/move/move.hpp>
#include <boost/optional.hpp>
#include <boost/thread/lock_types.hpp>
#include <boost/utility.hpp>

#include <fibio/fibers/future/future_status.hpp>
#include <fibio/fibers/condition_variable.hpp>
#include <fibio/fibers/exceptions.hpp>
#include <fibio/fibers/mutex.hpp>

namespace fibio { namespace fibers {
    typedef std::chrono::steady_clock clock_type;
}}

namespace fibio { namespace fibers { namespace detail {
    template< typename R >
    class shared_state : public boost::noncopyable
    {
    private:
        boost::atomic< std::size_t >   use_count_;
        mutable mutex                  mtx_;
        mutable condition_variable     waiters_;
        mutable std::mutex             state_mtx_;
        boost::atomic<bool>            ready_;
        boost::optional< R >           value_;
        boost::exception_ptr           except_;
        
        void mark_ready_and_notify_()
        {
            ready_ = true;
            waiters_.notify_all();
        }
        
        void owner_destroyed_()
        {
            //TODO: set broken_exception if future was not already done
            //      notify all waiters
            if (!ready_) {
                std::lock_guard<std::mutex> lock(state_mtx_);
                set_exception_( boost::copy_exception( broken_promise() ) );
            }
        }
        
        void set_value_( R const& value)
        {
            //TODO: store the value and make the future ready
            //      notify all waiters
            if (ready_)
                boost::throw_exception(promise_already_satisfied() );
            std::lock_guard<std::mutex> lock(state_mtx_);
            value_ = value;
            mark_ready_and_notify_();
        }
        
#ifndef BOOST_NO_RVALUE_REFERENCES
        void set_value_( R && value)
        {
            //TODO: store the value and make the future ready
            //      notify all waiters
            if (ready_)
                boost::throw_exception(promise_already_satisfied() );
            std::lock_guard<std::mutex> lock(state_mtx_);
            value_ = boost::move( value);
            mark_ready_and_notify_();
        }
#else
        void set_value_( BOOST_RV_REF( R) value)
        {
            //TODO: store the value and make the future ready
            //      notify all waiters
            if (ready_)
                boost::throw_exception(promise_already_satisfied() );
            std::lock_guard<std::mutex> lock(state_mtx_);
            value_ = boost::move( value);
            mark_ready_and_notify_();
        }
#endif
        
        void set_exception_( boost::exception_ptr except)
        {
            //TODO: store the exception pointer p into the shared state and make the state ready
            //      done = true, notify all waiters
            if (ready_)
                boost::throw_exception(promise_already_satisfied());
            std::lock_guard<std::mutex> lock(state_mtx_);
            except_ = except;
            mark_ready_and_notify_();
        }
        
        R get_( unique_lock< mutex > & lk)
        {
            //TODO: the get method waits until the future has a valid result and
            //      (depending on which template is used) retrieves it
            //      it effectively calls wait_() in order to wait for the result
            //      if it satisfies the requirements of MoveAssignable, the value is moved,
            //      otherwise it is copied
            wait_(lk);
            std::lock_guard<std::mutex> lock(state_mtx_);
            if (except_)
                rethrow_exception( except_);
            return value_.get();
        }
        
        void wait_( unique_lock< mutex > & lk) const
        {
            //TODO: blocks until the result becomes available
            while (!ready_)
                waiters_.wait(lk);
        }
        
        template< class Rep, class Period >
        future_status wait_for_( unique_lock< mutex > & lk,
                                std::chrono::duration< Rep, Period > const& timeout_duration) const
        {
            //TODO: blocks until the result becomes available or timeout
            while ( ! ready_)
            {
                cv_status st( waiters_.wait_for( lk, timeout_duration) );
                if ( cv_status::timeout == st && ! ready_)
                    return future_status::timeout;
            }
            return future_status::ready;
        }
        
        future_status wait_until_( unique_lock< mutex > & lk,
                                  clock_type::time_point const& timeout_time) const
        {
            //TODO: blocks until the result becomes available or timeout
            while ( ! ready_)
            {
                cv_status st( waiters_.wait_until( lk, timeout_time) );
                if ( cv_status::timeout == st && ! ready_)
                    return future_status::timeout;
            }
            return future_status::ready;
        }
        
    protected:
        virtual void deallocate_future() = 0;
        
    public:
        typedef boost::intrusive_ptr< shared_state >    ptr_t;
        
        shared_state() :
        use_count_( 0), mtx_(), ready_( false),
        value_(), except_()
        {}
        
        virtual ~shared_state() {}
        
        void owner_destroyed()
        {
            //TODO: lock mutex
            //      set broken_exception if future was not already done
            //      done = true, notify all waiters
            //unique_lock< mutex > lk( mtx_);
            owner_destroyed_();
        }
        
        void set_value( R const& value)
        {
            //TODO: store the value into the shared state and make the state ready
            //      the operation is atomic, i.e. it behaves as though they acquire a single mutex
            //      associated with the promise object while updating the promise object
            //      an exception is thrown if there is no shared state or the shared state already
            //      stores a value or exception
            //unique_lock< mutex > lk( mtx_);
            set_value_( value);
        }
        
#ifndef BOOST_NO_RVALUE_REFERENCES
        void set_value( R && value)
        {
            //TODO: store the value into the shared state and make the state ready
            //      rhe operation is atomic, i.e. it behaves as though they acquire a single mutex
            //      associated with the promise object while updating the promise object
            //      an exception is thrown if there is no shared state or the shared state already
            //      stores a value or exception
            //unique_lock< mutex > lk( mtx_);
            set_value_( boost::move( value) );
        }
#else
        void set_value( BOOST_RV_REF( R) value)
        {
            //TODO: store the value into the shared state and make the state ready
            //      rhe operation is atomic, i.e. it behaves as though they acquire a single mutex
            //      associated with the promise object while updating the promise object
            //      an exception is thrown if there is no shared state or the shared state already
            //      stores a value or exception
            //unique_lock< mutex > lk( mtx_);
            set_value_( boost::move( value) );
        }
#endif
        
        void set_exception( boost::exception_ptr except)
        {
            //TODO: store the exception pointer p into the shared state and make the state ready
            //      the operation is atomic, i.e. it behaves as though they acquire a single mutex
            //      associated with the promise object while updating the promise object
            //      an exception is thrown if there is no shared state or the shared state already
            //      stores a value or exception
            //unique_lock< mutex > lk( mtx_);
            set_exception_( except);
        }
        
        R get()
        {
            //TODO: the get method waits until the future has a valid result and
            //      (depending on which template is used) retrieves it
            //      it effectively calls wait() in order to wait for the result
            //      the value stored in the shared state
            //      if it satisfies the requirements of MoveAssignable, the value is moved,
            //      otherwise it is copied
            //      valid() == false after a call to this method.
            //      detect the case when valid == false before the call and throw a
            //      future_error with an error condition of future_errc::no_state
            unique_lock< mutex > lk( mtx_);
            return get_( lk);
        }
        
        void wait() const
        {
            //TODO: blocks until the result becomes available
            //      valid() == true after the call
            unique_lock< mutex > lk( mtx_);
            wait_( lk);
        }
        
        template< class Rep, class Period >
        future_status wait_for( std::chrono::duration< Rep, Period > const& timeout_duration) const
        {
            //TODO: blocks until the result becomes available or timeout
            //      valid() == true after the call
            unique_lock< mutex > lk( mtx_);
            return wait_for_( lk, timeout_duration);
        }
        
        future_status wait_until( clock_type::time_point const& timeout_time) const
        {
            //TODO: blocks until the result becomes available or timeout
            //      valid() == true after the call
            unique_lock< mutex > lk( mtx_);
            return wait_until_( lk, timeout_time);
        }
        
        void reset()
        { ready_ = false; }
        
        friend inline void intrusive_ptr_add_ref( shared_state * p) BOOST_NOEXCEPT
        { ++p->use_count_; }
        
        friend inline void intrusive_ptr_release( shared_state * p)
        {
            if ( 0 == --p->use_count_)
                p->deallocate_future();
        }
    };
    
    template< typename R >
    class shared_state< R & > : public boost::noncopyable
    {
    private:
        boost::atomic< std::size_t >   use_count_;
        mutable mutex           mtx_;
        mutable condition_variable       waiters_;
        mutable std::mutex             state_mtx_;
        boost::atomic<bool>              ready_;
        R                   *   value_;
        boost::exception_ptr           except_;
        
        void mark_ready_and_notify_()
        {
            ready_ = true;
            waiters_.notify_all();
        }
        
        void owner_destroyed_()
        {
            //TODO: set broken_exception if future was not already done
            //      notify all waiters
            if (!ready_) {
                std::lock_guard<std::mutex> lock(state_mtx_);
                set_exception_( boost::copy_exception( broken_promise() ) );
            }
        }
        
        void set_value_( R & value)
        {
            //TODO: store the value and make the future ready
            //      notify all waiters
            if (ready_)
                boost::throw_exception(promise_already_satisfied() );
            std::lock_guard<std::mutex> lock(state_mtx_);
            value_ = & value;
            mark_ready_and_notify_();
        }
        
        void set_exception_( boost::exception_ptr except)
        {
            //TODO: store the exception pointer p into the shared state and make the state ready
            //      done = true, notify all waiters
            if (ready_)
                boost::throw_exception(promise_already_satisfied() );
            std::lock_guard<std::mutex> lock(state_mtx_);
            except_ = except;
            mark_ready_and_notify_();
        }
        
        R & get_( unique_lock< mutex > & lk)
        {
            //TODO: the get method waits until the future has a valid result and
            //      (depending on which template is used) retrieves it
            //      it effectively calls wait_() in order to wait for the result
            //      if it satisfies the requirements of MoveAssignable, the value is moved,
            //      otherwise it is copied
            wait_(lk);
            std::lock_guard<std::mutex> lock(state_mtx_);
            if (except_)
                rethrow_exception(except_);
            return * value_;
        }
        
        void wait_( unique_lock< mutex > & lk) const
        {
            //TODO: blocks until the result becomes available
            while ( ! ready_)
                waiters_.wait( lk);
        }
        
        template< class Rep, class Period >
        future_status wait_for_( unique_lock< mutex > & lk,
                                std::chrono::duration< Rep, Period > const& timeout_duration) const
        {
            //TODO: blocks until the result becomes available or timeout
            while ( ! ready_)
            {
                cv_status st( waiters_.wait_for( lk, timeout_duration) );
                if ( cv_status::timeout == st && ! ready_)
                    return future_status::timeout;
            }
            return future_status::ready;
        }
        
        future_status wait_until_( unique_lock< mutex > & lk,
                                  clock_type::time_point const& timeout_time) const
        {
            //TODO: blocks until the result becomes available or timeout
            while ( ! ready_)
            {
                cv_status st( waiters_.wait_until( lk, timeout_time) );
                if ( cv_status::timeout == st && ! ready_)
                    return future_status::timeout;
            }
            return future_status::ready;
        }
        
    protected:
        virtual void deallocate_future() = 0;
        
    public:
        typedef boost::intrusive_ptr< shared_state >    ptr_t;
        
        shared_state() :
        use_count_( 0), mtx_(), ready_( false),
        value_( 0), except_()
        {}
        
        virtual ~shared_state() {}
        
        void owner_destroyed()
        {
            //TODO: lock mutex
            //      set broken_exception if future was not already done
            //      done = true, notify all waiters
            //unique_lock< mutex > lk( mtx_);
            owner_destroyed_();
        }
        
        void set_value( R & value)
        {
            //TODO: store the value into the shared state and make the state ready
            //      the operation is atomic, i.e. it behaves as though they acquire a single mutex
            //      associated with the promise object while updating the promise object
            //      an exception is thrown if there is no shared state or the shared state already
            //      stores a value or exception
            //unique_lock< mutex > lk( mtx_);
            set_value_( value);
        }
        
        void set_exception( boost::exception_ptr except)
        {
            //TODO: store the exception pointer p into the shared state and make the state ready
            //      the operation is atomic, i.e. it behaves as though they acquire a single mutex
            //      associated with the promise object while updating the promise object
            //      an exception is thrown if there is no shared state or the shared state already
            //      stores a value or exception
            //unique_lock< mutex > lk( mtx_);
            set_exception_( except);
        }
        
        R & get()
        {
            //TODO: the get method waits until the future has a valid result and
            //      (depending on which template is used) retrieves it
            //      it effectively calls wait() in order to wait for the result
            //      the value stored in the shared state
            //      if it satisfies the requirements of MoveAssignable, the value is moved,
            //      otherwise it is copied
            //      valid() == false after a call to this method.
            //      detect the case when valid == false before the call and throw a
            //      future_error with an error condition of future_errc::no_state
            unique_lock< mutex > lk( mtx_);
            return get_( lk);
        }
        
        void wait() const
        {
            //TODO: blocks until the result becomes available
            //      valid() == true after the call
            unique_lock< mutex > lk( mtx_);
            wait_( lk);
        }
        
        template< class Rep, class Period >
        future_status wait_for( std::chrono::duration< Rep, Period > const& timeout_duration) const
        {
            //TODO: blocks until the result becomes available or timeout
            //      valid() == true after the call
            unique_lock< mutex > lk( mtx_);
            return wait_for_( lk, timeout_duration);
        }
        
        future_status wait_until( clock_type::time_point const& timeout_time) const
        {
            //TODO: blocks until the result becomes available or timeout
            //      valid() == true after the call
            unique_lock< mutex > lk( mtx_);
            return wait_until_( lk, timeout_time);
        }
        
        void reset()
        { ready_ = false; }
        
        friend inline void intrusive_ptr_add_ref( shared_state * p) BOOST_NOEXCEPT
        { ++p->use_count_; }
        
        friend inline void intrusive_ptr_release( shared_state * p)
        {
            if ( 0 == --p->use_count_)
                p->deallocate_future();
        }
    };
    
    template<>
    class shared_state< void > : public boost::noncopyable
    {
    private:
        boost::atomic< std::size_t >   use_count_;
        mutable mutex           mtx_;
        mutable condition_variable       waiters_;
        std::mutex                     state_mtx_;
        boost::atomic<bool>              ready_;
        boost::exception_ptr           except_;
        
        void mark_ready_and_notify_()
        {
            ready_ = true;
            waiters_.notify_all();
        }
        
        void owner_destroyed_()
        {
            //TODO: set broken_exception if future was not already done
            //      notify all waiters
            if (!ready_) {
                std::lock_guard<std::mutex> lock(state_mtx_);
                set_exception_(boost::copy_exception( broken_promise() ) );
            }
        }
        
        void set_value_()
        {
            //TODO: store the value and make the future ready
            //      notify all waiters
            if (ready_)
                boost::throw_exception(promise_already_satisfied() );
            std::lock_guard<std::mutex> lock(state_mtx_);
            mark_ready_and_notify_();
        }
        
        void set_exception_( boost::exception_ptr except)
        {
            //TODO: store the exception pointer p into the shared state and make the state ready
            //      done = true, notify all waiters
            if (ready_)
                boost::throw_exception(promise_already_satisfied() );
            std::lock_guard<std::mutex> lock(state_mtx_);
            except_ = except;
            mark_ready_and_notify_();
        }
        
        void get_( unique_lock< mutex > & lk)
        {
            //TODO: the get method waits until the future has a valid result and
            //      (depending on which template is used) retrieves it
            //      it effectively calls wait_() in order to wait for the result
            //      if it satisfies the requirements of MoveAssignable, the value is moved,
            //      otherwise it is copied
            wait_(lk);
            std::lock_guard<std::mutex> lock(state_mtx_);
            if (except_)
                rethrow_exception( except_);
        }
        
        void wait_(unique_lock< mutex > & lk) const
        {
            //TODO: blocks until the result becomes available
            while ( ! ready_)
                waiters_.wait( lk);
        }
        
        template< class Rep, class Period >
        future_status wait_for_( unique_lock< mutex > & lk,
                                std::chrono::duration< Rep, Period > const& timeout_duration) const
        {
            //TODO: blocks until the result becomes available or timeout
            while ( ! ready_)
            {
                cv_status st( waiters_.wait_for( lk, timeout_duration) );
                if ( cv_status::timeout == st && ! ready_)
                    return future_status::timeout;
            }
            return future_status::ready;
        }
        
        future_status wait_until_( unique_lock< mutex > & lk,
                                  clock_type::time_point const& timeout_time) const
        {
            //TODO: blocks until the result becomes available or timeout
            while ( ! ready_)
            {
                cv_status st( waiters_.wait_until( lk, timeout_time) );
                if ( cv_status::timeout == st && ! ready_)
                    return future_status::timeout;
            }
            return future_status::ready;
        }
        
    protected:
        virtual void deallocate_future() = 0;
        
    public:
        typedef boost::intrusive_ptr< shared_state >    ptr_t;
        
        shared_state() :
        use_count_( 0), mtx_(), ready_( false), except_()
        {}
        
        virtual ~shared_state() {}
        
        void owner_destroyed()
        {
            //TODO: lock mutex
            //      set broken_exception if future was not already done
            //      done = true, notify all waiters
            //unique_lock< mutex > lk( mtx_);
            owner_destroyed_();
        }
        
        void set_value()
        {
            //TODO: store the value into the shared state and make the state ready
            //      the operation is atomic, i.e. it behaves as though they acquire a single mutex
            //      associated with the promise object while updating the promise object
            //      an exception is thrown if there is no shared state or the shared state already
            //      stores a value or exception
            //unique_lock< mutex > lk( mtx_);
            set_value_();
        }
        
        void set_exception( boost::exception_ptr except)
        {
            //TODO: store the exception pointer p into the shared state and make the state ready
            //      the operation is atomic, i.e. it behaves as though they acquire a single mutex
            //      associated with the promise object while updating the promise object
            //      an exception is thrown if there is no shared state or the shared state already
            //      stores a value or exception
            //unique_lock< mutex > lk( mtx_);
            set_exception_( except);
        }
        
        void get()
        {
            //TODO: the get method waits until the future has a valid result and
            //      (depending on which template is used) retrieves it
            //      it effectively calls wait() in order to wait for the result
            //      the value stored in the shared state
            //      if it satisfies the requirements of MoveAssignable, the value is moved,
            //      otherwise it is copied
            //      valid() == false after a call to this method.  
            //      detect the case when valid == false before the call and throw a
            //      future_error with an error condition of future_errc::no_state
            unique_lock< mutex > lk( mtx_);
            get_( lk);
        }
        
        void wait() const
        {
            //TODO: blocks until the result becomes available
            //      valid() == true after the call
            unique_lock< mutex > lk( mtx_);
            wait_( lk);
        }
        
        template< class Rep, class Period >
        future_status wait_for( std::chrono::duration< Rep, Period > const& timeout_duration) const
        {
            //TODO: blocks until the result becomes available or timeout
            //      valid() == true after the call
            unique_lock< mutex > lk( mtx_);
            return wait_for_( lk, timeout_duration);
        }
        
        future_status wait_until( clock_type::time_point const& timeout_time) const
        {
            //TODO: blocks until the result becomes available or timeout
            //      valid() == true after the call
            unique_lock< mutex > lk( mtx_);
            return wait_until_( lk, timeout_time);
        }
        
        void reset()
        { ready_ = false; }
        
        friend inline void intrusive_ptr_add_ref( shared_state * p) BOOST_NOEXCEPT
        { ++p->use_count_; }
        
        friend inline void intrusive_ptr_release( shared_state * p)
        {
            if ( 0 == --p->use_count_)
                p->deallocate_future();
        }
    };
}}} // End of namespace fibio::fibers::detail

#endif
