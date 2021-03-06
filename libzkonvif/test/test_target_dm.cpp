﻿/**	Zonekey Source:
		实现 target 端的 device mgrt 接口
 */

#include "../../common/log.h"
#include "myservice.inf.h"
#include "MyEvent.h"
#include "MyPtz.h"
#include "MyDevice.h"
#include "MyDeviceDiscovery.h"
#ifdef WIN32
#	include "../src/win/sysperf.h"
#else
#	include "../src/linux/sysperf.h"
#endif // 
#include <cc++/socketport.h>

#ifdef WITH_OPENSSL

#ifdef _POSIX_THREADS 
# include <pthread.h>
#endif 
#if defined(WIN32) 
# define MUTEX_TYPE HANDLE 
# define MUTEX_SETUP(x) (x) = CreateMutex(NULL, FALSE, NULL) 
# define MUTEX_CLEANUP(x) CloseHandle(x) 
# define MUTEX_LOCK(x) WaitForSingleObject((x), INFINITE) 
# define MUTEX_UNLOCK(x) ReleaseMutex(x) 
# define THREAD_ID GetCurrentThreadId() 
#elif defined(_POSIX_THREADS) 
# define MUTEX_TYPE pthread_mutex_t 
# define MUTEX_SETUP(x) pthread_mutex_init(&(x), NULL) 
# define MUTEX_CLEANUP(x) pthread_mutex_destroy(&(x)) 
# define MUTEX_LOCK(x) pthread_mutex_lock(&(x)) 
# define MUTEX_UNLOCK(x) pthread_mutex_unlock(&(x)) 
# define THREAD_ID pthread_self() 
#else 
# error "You must define mutex operations appropriate for your platform" 
# error "See OpenSSL /threads/th-lock.c on how to implement mutex on your platform" 
#endif 

struct CRYPTO_dynlock_value { MUTEX_TYPE mutex; };
static MUTEX_TYPE *mutex_buf;

static struct CRYPTO_dynlock_value *dyn_create_function(const char *file, int line)
{
	struct CRYPTO_dynlock_value *value;
	value = (struct CRYPTO_dynlock_value*)malloc(sizeof(struct CRYPTO_dynlock_value));
	if (value)
		MUTEX_SETUP(value->mutex);
	return value;
}
static void dyn_lock_function(int mode, struct CRYPTO_dynlock_value *l, const char *file, int line)
{
	if (mode & CRYPTO_LOCK)
		MUTEX_LOCK(l->mutex);
	else
		MUTEX_UNLOCK(l->mutex);
}
static void dyn_destroy_function(struct CRYPTO_dynlock_value *l, const char *file, int line)
{
	MUTEX_CLEANUP(l->mutex);
	free(l);
}
void locking_function(int mode, int n, const char *file, int line)
{
	if (mode & CRYPTO_LOCK)
		MUTEX_LOCK(mutex_buf[n]);
	else
		MUTEX_UNLOCK(mutex_buf[n]);
}
unsigned long id_function()
{
	return (unsigned long)THREAD_ID;
}
int CRYPTO_thread_setup()
{
	int i;
	mutex_buf = (MUTEX_TYPE*)malloc(CRYPTO_num_locks() * sizeof(MUTEX_TYPE));
	if (!mutex_buf)
		return SOAP_EOM;
	for (i = 0; i < CRYPTO_num_locks(); i++)
		MUTEX_SETUP(mutex_buf[i]);
	CRYPTO_set_id_callback(id_function);
	CRYPTO_set_locking_callback(locking_function);
	CRYPTO_set_dynlock_create_callback(dyn_create_function);
	CRYPTO_set_dynlock_lock_callback(dyn_lock_function);
	CRYPTO_set_dynlock_destroy_callback(dyn_destroy_function);
	return SOAP_OK;
}
void CRYPTO_thread_cleanup()
{
	int i;
	if (!mutex_buf)
		return;
	CRYPTO_set_id_callback(NULL);
	CRYPTO_set_locking_callback(NULL);
	CRYPTO_set_dynlock_create_callback(NULL);
	CRYPTO_set_dynlock_lock_callback(NULL);
	CRYPTO_set_dynlock_destroy_callback(NULL);
	for (i = 0; i < CRYPTO_num_locks(); i++)
		MUTEX_CLEANUP(mutex_buf[i]);
	free(mutex_buf);
	mutex_buf = NULL;
}
#endif // with openssl

/** rtmp 服务接口, 这个仅仅为了演示如何使用 ServiceInf
*/
class MyMediaStream : public ServiceInf
				    , ost::Thread
{
	std::string url_;

public:
	MyMediaStream()
	{
		url_ = "rtmp://0.0.0.0:0"; // FIXME: 一个非法的 url :)
		start();
	}

private:
	const char *url() const { return url_.c_str(); }
	const char *desc() const { return "zonekey RTMP living cast ...!"; }
	const char *ns() const
	{
		// FIXME: 这里应该照着规矩来 ...
		return "media";
	}
	const char *sid() const
	{
		//FIXME:应该用mac +ns+ id的形式作为唯一标识，这里先把mac默认为00000000000000
		return "000000000000media0";
	}

private:
	void run()
	{
		int code = 0;

		/// 测试：每隔1000豪秒，发出一个通知 ...
		while (0) {
			sink_->post(this->ns(), "test", code++, "...");
			sleep(1000);
		}
	}

private:
	static const char* my_messageid()
	{
		static int _i = 0;
		static char buf[64];

		snprintf(buf, sizeof(buf), "id:%u", _i++);

		return buf;
	}
};

class MyService : ost::SocketService
{
public:

};

class MySocket : ost::SocketPort
{
public:
	MySock
};

static void test_sss()
{
	ost::IPV4Address addr;
	ost::TCPSocket ss(addr, 20000);


}

int main2(int argc, char **argv)
{
	test_sss();
	log_init();

//	ost::Thread::setStack(16384);
//	fprintf(stdout, "en, using thread stack size: %u\n", 16384);

	// 初始化 openssl ...
#ifdef WITH_OPENSSL
	soap_ssl_init();
	CRYPTO_thread_setup();
#endif

#ifdef WIN32
	SysPerf sp("c:", util_get_nic_name());
#else
	SysPerf sp("/home", util_get_nic_name());
#endif

	std::vector<ServiceInf *> services;

	MyEvent evt(10002);	// 事件服务 ...
	services.push_back(&evt);

	MyPtz ptz(10003);	// 云台服务 ..
	ptz.set_eventsink(&evt);
	services.push_back(&ptz);

	MyMediaStream ms;	// 直播服务 ..
	ms.set_eventsink(&evt);
	services.push_back(&ms);

	MyDevice device(10000, services);
	MyDeviceDiscovery discovery(&device);

	while (1) {
		ost::Thread::sleep(100);
	}

	return 0;
}
