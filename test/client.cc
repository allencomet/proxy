#include "../proxy/proxy.h"
#include "../client/client.h"

#include "../util/util.h"

namespace test {

enum ACTION {
	kStart,
	kStop
};

boost::shared_ptr<proxy::rpc::NetServer> srv;
safe::SyncEvent ev;

void start_proxy_server(ACTION action) {
	if (kStart == action) {
		if (srv) {
			srv->stop();
			srv.reset(new proxy::rpc::NetServer("*", 8080));
			srv->run();
		} else {
			srv.reset(new proxy::rpc::NetServer("*", 8080));
			srv->run();
		}
	} else {
		if (srv) {
			srv->stop();
			srv.reset();
			ev.signal();
		}
	}
}

void start_proxy_client() {
	int fd = client::rpc_client("127.0.0.1", 8080);
	client::start_rpc_client(fd);

	//start_proxy_server(kStop);
}

void start_monitor(int sec) {
	sys::sleep(sec);
	start_proxy_server(kStop);
}

DEF_test(proxy) {
	DEF_case(rpc_serv_cli) {
		safe::Thread serv(boost::bind(&start_proxy_server, kStart));
		serv.start();

		sys::msleep(500);//等待服务器初始化完成

		safe::Thread cli(boost::bind(&start_proxy_client));
		cli.start();

		safe::Thread monitor(boost::bind(&start_monitor,10));
		monitor.start();

		/*std::vector<int32> pids = os::check_process("backend");
		for (std::vector<int32>::iterator itp = pids.begin(); itp != pids.end(); ++itp) {
			::kill(*itp, SIGKILL);
			COUT << "kill [path:backend][pid:" << *itp << "]...";
		}*/

		ev.wait();
		COUT << "wait for server stop running...";
		serv.cancel();
		serv.join();
		COUT << "wait for client stop running...";
		cli.cancel();
		cli.join();
		COUT << "wait for monitor stop running...";
		monitor.cancel();
		monitor.join();
	}
}

}