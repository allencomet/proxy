#include "../proxy/proxy.h"
#include "../client/client.h"

#include "../util/util.h"

namespace test {

enum ACTION {
	kStart,
	kStop
};

boost::shared_ptr<proxy::rpc::NetServer> rpcsrv;
boost::shared_ptr<proxy::ipc::LocalServer> ipcsrv;
safe::SyncEvent ev;

void start_proxy_server(ACTION action) {
	if (kStart == action) {
		if (rpcsrv) {
			rpcsrv->stop();
			rpcsrv.reset(new proxy::rpc::NetServer("*", 8080));
			rpcsrv->run();
		} else {
			rpcsrv.reset(new proxy::rpc::NetServer("*", 8080));
			rpcsrv->run();
		}
	} else {
		if (rpcsrv) {
			rpcsrv->stop();
			rpcsrv.reset();
			
		}
		ev.signal();
	}
}

void start_backend_server(ACTION action) {
	if (kStart == action) {
		if (ipcsrv) {
			ipcsrv->stop();
			ipcsrv.reset(new proxy::ipc::LocalServer(::FLG_remotepath));
			ipcsrv->run();
		} else {
			ipcsrv.reset(new proxy::ipc::LocalServer(::FLG_remotepath));
			ipcsrv->run();
		}
	} else {
		if (rpcsrv) {
			ipcsrv->stop();
			ipcsrv.reset();
		}
	}
}

void start_proxy_client() {
	int fd = client::rpc_client("127.0.0.1", 8080);
	client::start_rpc_client(fd);
}

void start_monitor(int sec) {
	sys::sleep(sec);
	start_proxy_server(kStop);
	start_backend_server(kStop);
}

DEF_test(proxy) {
	DEF_case(rpc_serv_cli) {
		safe::Thread back(boost::bind(&start_backend_server, kStart));
		back.start();

		safe::Thread agent(boost::bind(&start_proxy_server, kStart));
		agent.start();

		sys::msleep(500);//等待服务器初始化完成

		safe::Thread cli(boost::bind(&start_proxy_client));
		cli.start();

		safe::Thread monitor(boost::bind(&start_monitor,10));
		monitor.start();

		ev.wait();
		COUT << "wait for proxy server stop running...";
		agent.cancel();
		agent.join();
		COUT << "wait for back server stop running...";
		back.cancel();
		back.join();
		COUT << "wait for client stop running...";
		cli.cancel();
		cli.join();
		COUT << "wait for monitor stop running...";
		monitor.cancel();
		monitor.join();
	}
}

}