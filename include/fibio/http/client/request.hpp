//
//  request.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-10.
//  Copyright (c) 2014年 0d0a.com. All rights reserved.
//

#ifndef fiberized_io_http_client_request_hpp
#define fiberized_io_http_client_request_hpp

#include <string>
#include <boost/interprocess/streams/vectorstream.hpp>
#include <fibio/http/common/request.hpp>
#include <fibio/http/common/content_type.hpp>

namespace fibio { namespace http { namespace client {
    struct request : common::request {
        request()=default;
        
        request(const request &other)
        : common::request(other)
        {}
        
        request &operator=(const request &other) {
            common::request::operator=(other);
            return *this;
        }
        
        void clear();
        
        size_t get_content_length() const;
        
        void set_compression(bool);
        
        bool get_compression() const;

        std::ostream &body_stream();
        
        template<typename T>
        void set_body(const T &t, const std::string &content_type=common::content_type<T>::name) {
            body_stream() << t;
            if (get_content_type().empty())
                set_content_type(content_type);
        }

        bool write(std::ostream &os);

        boost::interprocess::basic_ovectorstream<std::string> raw_body_stream_;
    };
}}} // End of namespace fibio::http::client

#endif
