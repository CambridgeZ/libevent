/*
 * Copyright (c) 2000-2007 Niels Provos <provos@citi.umich.edu>
 * Copyright (c) 2007-2012 Niels Provos and Nick Mathewson
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef EVENT2_BUFFEREVENT_STRUCT_H_INCLUDED_
#define EVENT2_BUFFEREVENT_STRUCT_H_INCLUDED_

/** @file event2/bufferevent_struct.h

  Data structures for bufferevents.  Using these structures may hurt forward
  compatibility with later versions of Libevent: be careful!

  @deprecated Use of bufferevent_struct.h is completely deprecated; these
    structures are only exposed for backward compatibility with programs
    written before Libevent 2.0 that used them.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <event2/event-config.h>
#ifdef EVENT__HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef EVENT__HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

/* For int types. */
#include <event2/util.h>
/* For struct event */
#include <event2/event_struct.h>

struct event_watermark {
	// 低水位
	size_t low;
	//高水位
	size_t high;
};// 数据水位

/**
  Shared implementation of a bufferevent.

  This type is exposed only because it was exposed in previous versions,
  and some people's code may rely on manipulating it.  Otherwise, you
  should really not rely on the layout, size, or contents of this structure:
  it is fairly volatile, and WILL change in future versions of the code.
**/
struct bufferevent {
	/** Event base for which this bufferevent was created. */
	//实例指针
	struct event_base *ev_base;
	/** Pointer to a table of function pointers to set up how this
	    bufferevent behaves. */
		    // bufferevent_ops包含几个函数指针，这里限于篇幅，只讨论最关键的几个
    // be_ops->enable:我们需要通过bufferevent_enable函数，来注册bufferevent的读写事件
    // 在调用它的时候，be_ops->enable函数会被调用，添加读写event到evmap_io_map的逻辑，就
    // 是它来实现的。
    // be_ops->disable:我们需要通过bufferevent_disable函数，来删除bufferevent实例的读写事件
    // 调用它的时候，be_ops->disable函数会被调用，从evmap_io_map中删除读写event的逻辑，就是它执行的
    // be_ops->destruct:在销毁bufferevent实例的时候调用，主要的操作是调用close api，关闭socket实例
	const struct bufferevent_ops *be_ops;//包含了几个函数指针

	/** A read event that triggers when a timeout has happened or a socket
	    is ready to read data.  Only used by some subtypes of
	    bufferevent. */
		//可读时间触发的时候会调用的函数
	struct event ev_read;
	/** A write event that triggers when a timeout has happened or a socket
	    is ready to write data.  Only used by some subtypes of
	    bufferevent. */
		//可写时间触发的时候会调用的函数
	struct event ev_write;

	/** An input buffer. Only the bufferevent is allowed to add data to
	    this buffer, though the user is allowed to drain it. */
	// 当ev_read事件的callback函数被调用时，它会主动读取数据，并存入input的缓存中，evbuffer
    // 实例，内部有个锁变量，在多线程使用情形中，保证线程安全。缓存是个链表结构。
	struct evbuffer *input;

	/** An output buffer. Only the bufferevent is allowed to drain data
	    from this buffer, though the user is allowed to add it. */
	// 当ev_write事件的callback函数被调用时，他会主动写数据到outpur缓存中，evbuffer实例，内
    // 部有个锁变量，在多线程使用情形中，保证线程安全。缓存是个链表结构。
	struct evbuffer *output;
	// 读取数据水位标记：当input缓存的数据，高于wm_read.low时，且低于wm_read.high时，
    // bufferevent->readcb会被调用。默认low和high都是0，表示一有数据在input缓冲，readcb就会被
    // 调用。我们讨论的情景，均是默认的情况。
	struct event_watermark wm_read;
	// 写数据的水位标记：当output缓存的数据，低于wm_write.low时，bufferevent->writecb会被调用
    // 默认值是0，因此只有output缓冲被处理完时，它才会被调用。我们讨论的情景，均是默认的情况。
	struct event_watermark wm_write;

	// 用户注册的读取事件，通过bufferevent_setcb函数注册。一般情况下，IO读取事件准备就绪后，ev_read
    // 的回调函数会被调用，这个回调函数是libevent内部定义的一个，将数据读取，并放入input缓存的函数，
    // 在这个操作完成之后，就会调用readcb函数，用来告知用户，有数据到来了，并且到来多少个字节，用户可以
    // 直接从input buffer获取。
	bufferevent_data_cb readcb;
	// 用户注册的写事件，通过bufferevent_setcb函数注册。一般情况下，IO写事件准备就绪之后，ev_write的
    // 回调函数会被调用，这个回调函数时libevent内部定义的一个函数，它将output buffer数据取出，并且调用
    // 系统write函数写数据。当output buffer的剩余字节数小于等于wm_write.low的时候，writecb函数被调用。
	bufferevent_data_cb writecb;
	/* This should be called 'eventcb', but renaming it would break
	 * backward compatibility */
	// 用户注册的错误处理事件，通过bufferevent_setcb函数注册。一般有错误会通过这个函数抛出，我们可以在
    // 这个函数被回调时，关闭连接。
	bufferevent_event_cb errorcb;
	void *cbarg;

	struct timeval timeout_read;
	struct timeval timeout_write;

	/** Events that are currently enabled: currently EV_READ and EV_WRITE
	    are supported. */
	short enabled;
};

#ifdef __cplusplus
}
#endif

#endif /* EVENT2_BUFFEREVENT_STRUCT_H_INCLUDED_ */
