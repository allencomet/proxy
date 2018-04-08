#ifndef __NET_MT_LB_LOADBALANCER_H
#define __NET_MT_LB_LOADBALANCER_H

/************************************************************************/
/* 
  ���ؾ����ַ�Ϊ�Ĳ㸺�ؾ�����߲㸺�ؾ��⡣�Ĳ㸺�ؾ��⹤����OSIģ�͵Ĵ���㣬��Ҫ������ת�������ڽ��յ�
�ͻ��˵������Ժ�ͨ���޸����ݰ��ĵ�ַ��Ϣ������ת����Ӧ�÷�������

���߲㸺�ؾ��⹤����OSIģ�͵�Ӧ�ò㣬��Ϊ����Ҫ����Ӧ�ò������������߲㸺�ؾ����ڽӵ��ͻ��˵������Ժ󣬻�
��Ҫһ��������TCP/IPЭ��ջ���߲㸺�ؾ������ͻ��˽���һ�����������Ӳ���Ӧ�ò���������������������ٰ��յ�
���㷨ѡ��һ��Ӧ�÷�����������Ӧ�÷�������������һ�����ӽ������͹�ȥ������߲㸺�ؾ������Ҫ�������Ǵ���

����ԭ���ϵ�����:
  ��ν�Ĳ㸺�ؾ��⣬Ҳ������Ҫͨ�������е�Ŀ���ַ�Ͷ˿ڣ��ټ��ϸ��ؾ����豸���õķ�����ѡ��ʽ����������ѡ����ڲ���������

  �Գ�����TCPΪ�������ؾ����豸�ڽ��յ���һ�����Կͻ��˵�SYN ����ʱ����ͨ��������ʽѡ��һ����ѵķ����������Ա�����Ŀ��IP��
ַ�����޸�(��Ϊ��˷�����IP����ֱ��ת�����÷�������TCP�����ӽ����������������ǿͻ��˺ͷ�����ֱ�ӽ����ģ����ؾ����豸ֻ����
һ������·������ת����������ĳЩ��������£�Ϊ��֤�������ذ�������ȷ���ظ����ؾ����豸����ת�����ĵ�ͬʱ���ܻ���Ա���ԭ����
Դ��ַ�����޸ġ�

  ��ν�߲㸺�ؾ��⣬Ҳ��Ϊ�����ݽ�������Ҳ������Ҫͨ�������е������������Ӧ�ò����ݣ��ټ��ϸ��ؾ����豸���õķ�����ѡ��ʽ��
��������ѡ����ڲ���������
  �Գ�����TCPΪ�������ؾ����豸���Ҫ����������Ӧ�ò�������ѡ���������ֻ���ȴ������յķ������Ϳͻ��˽�������(��������)�󣬲�
���ܽ��ܵ��ͻ��˷��͵�����Ӧ�ò����ݵı��ģ�Ȼ���ٸ��ݸñ����е��ض��ֶΣ��ټ��ϸ��ؾ����豸���õķ�����ѡ��ʽ����������ѡ��
���ڲ���������
  ���ؾ����豸����������£���������һ����������������ؾ����ǰ�˵Ŀͻ����Լ���˵ķ�������ֱ���TCP���ӡ����Դ��������ԭ
�����������߲㸺�ؾ������ԵĶԸ��ؾ����豸��Ҫ����ߣ������߲������Ҳ��Ȼ������Ĳ�ģʽ�Ĳ���ʽ��

Ӧ�ó���������:
  �߲�Ӧ�ø��صĺô�����ʹ�����������"���ܻ�"���������һ����վ���û�����������ͨ���߲�ķ�ʽ������ͼƬ�������ת�����ض���ͼ
Ƭ������������ʹ�û��漼����������������������ת�����ض������ַ�����������ʹ��ѹ��������
  ��Ȼ��ֻ���߲�Ӧ�õ�һ��С�������Ӽ���ԭ���ϣ����ַ�ʽ���ԶԿͻ��˵�����ͷ���������Ӧ�������������ϵ��޸ģ������������Ӧ��
ϵͳ������������ԡ��ܶ��ں�̨��(����Nginx����Apache)�ϲ���Ĺ��ܿ���ǰ�Ƶ����ؾ����豸�ϣ�����ͻ������е�Header��д������
����Ӧ�еĹؼ��ֹ��˻������ݲ���ȹ��ܡ�

  ����һ���������ᵽ���ܾ��ǰ�ȫ�ԡ������������SYN Flood���������ڿͿ����ڶ�Դ�ͻ��ˣ�ʹ�����IP��ַ��ͬһĿ�귢��SYN������ͨ
�����ֹ������������SYN���ģ��ľ��������ϵ������Դ���ԴﵽDenial of Service(DoS)��Ŀ�ġ�

���Ӽ���ԭ����Ҳ���Կ������Ĳ�ģʽ����ЩSYN�������ᱻת������˵ķ������ϣ����߲�ģʽ����ЩSYN������Ȼ�ڸ��ؾ����豸�Ͼͽ�ֹ����
��Ӱ���̨��������������Ӫ�����⸺�ؾ����豸�������߲�����趨���ֲ��ԣ������ض����ģ�����SQL Injection��Ӧ�ò�����ض������ֶΣ�
��Ӧ�ò����һ�����ϵͳ���尲ȫ��
  ���ڵ�7�㸺�ؾ��⣬��Ҫ����������Ӧ�ù㷺��HTTPЭ�飬������Ӧ�÷�Χ��Ҫ���ڶ����վ�����ڲ���Ϣƽ̨�Ȼ���B/S������ϵͳ�� 4�㸺
�ؾ������Ӧ����TCPӦ�ã��������C/S������ERP��ϵͳ��


�߲�Ӧ����Ҫ���ǵ�����:
(1)�Ƿ���ı�Ҫ���߲�Ӧ�õ�ȷ��������������ܻ���ͬʱ�ز�����Ĵ����豸���ø��ӣ����ؾ���ѹ�������Լ������Ų��ϵĸ����Ե����⡣��
���ϵͳʱ��Ҫ�����Ĳ��߲�ͬʱӦ�õĻ��������
(2)�Ƿ���Ŀ�����߰�ȫ�ԡ�����SYN Flood�������߲�ģʽ��ȷ����Щ�����ӷ��������Σ������ؾ����豸����Ҫ��ǿ��Ŀ�DDoS����������ʹ
��������������Ϊ������ȵĸ��ؾ����豸����Ҳ�ᵼ������Ӧ�õı�����
(3)�Ƿ����㹻�����ȡ��߲�Ӧ�õ������ǿ���������Ӧ�õ��������ܻ������Ǹ��ؾ����豸��Ҫ�ṩ���Ƶ��߲㹦�ܣ�����ͻ����ݲ�ͬ����Ļ�
��Ӧ�õĵ��ȡ���򵥵�һ�����˾����ܷ�ȡ����̨Nginx����Apache�ȷ������ϵĵ��ȹ��ܡ��ܹ��ṩһ���߲�Ӧ�ÿ����ӿڵĸ��ؾ����豸������
�ÿͻ��������������趨���ܣ��������п����ṩǿ�������Ժ������ԡ�

���ؾ�����㷨:
1. ����㷨
  Random�������Ȩ������������ʡ���һ����������ײ�ĸ��ʸߣ���������Խ��ֲ�Խ���ȣ�
���Ұ�����ʹ��Ȩ�غ�Ҳ�ȽϾ��ȣ������ڶ�̬�����ṩ��Ȩ�ء�

2. ��ѯ����Ȩ��ѯ
  ��ѯ(Round Robbin)��������Ⱥ�и��������Ĵ���������ͬʱ����ÿ��ҵ���������첻��ʱ�����ʺ�ʹ�������㷨�� 
��ѭ������Լ���Ȩ��������ѭ���ʡ����������ṩ���ۻ��������⣬���磺�ڶ�̨������������û�ң������������
��̨ʱ�Ϳ����ǣ��ö���֮���������󶼿��ڵ����ڶ�̨�ϡ�
  ��Ȩ��ѯ(Weighted Round Robbin)Ϊ��ѯ�е�ÿ̨����������һ��Ȩ�ص��㷨�����������1Ȩ��1��������2Ȩ��2��
������3Ȩ��3����˳��Ϊ1-2-2-3-3-3-1-2-2-3-3-3- ......


3. ��С���Ӽ���Ȩ��С����
  ��������(Least Connections)�ڶ���������У��봦��������(�Ự��)���ٵķ���������ͨ�ŵ��㷨����ʹ��ÿ̨����
����������������ͬ��ÿ��ҵ������Ҳ����ͬ������£�Ҳ�ܹ���һ���̶��Ͻ��ͷ������ĸ��ء�
  ��Ȩ��������(Weighted Least Connection)Ϊ���������㷨�е�ÿ̨����������Ȩ�ص��㷨�����㷨����Ϊÿ̨������
���䴦�����ӵ������������ͻ�������ת�����������ٵķ������ϡ�

4. ��ϣ�㷨
  ��ͨ��ϣ
  һ���Թ�ϣһ����Hash����ͬ�������������Ƿ���ͬһ�ṩ�ߡ���ĳһ̨�ṩ�߹�ʱ��ԭ���������ṩ�ߵ����󣬻�����
��ڵ㣬ƽ̯�������ṩ�ߣ�����������ұ䶯��

5. IP��ַɢ��
  ͨ�������ͷ�IP��Ŀ�ĵ�IP��ַ��ɢ�У�������ͬһ���ͷ��ķ���(������ͬһĿ�ĵصķ���)ͳһת������ͬ������
���㷨�����ͻ�����һϵ��ҵ����Ҫ����������һ������������ͨ��ʱ�����㷨�ܹ�����(�Ự)Ϊ��λ����֤������ͬ
�ͻ��˵�ͨ���ܹ�һֱ��ͬһ�������н��д���

6.URLɢ��
  ͨ������ͻ�������URL��Ϣ��ɢ�У�����������ͬURL������ת����ͬһ���������㷨��


���ؾ����ʵ�֣�
1 - DNS�����������ؾ��⣨�ӳ٣�
  ����DNS�����������������ͬʱ���и��ؾ�������һ�ֳ��õķ�������DNS�����������ö��A��¼��
�磺www.mysite.com IN A 114.100.80.1��www.mysite.com IN A 114.100.80.2��www.mysite.com IN A 114.100.80.3.
ÿ�������������󶼻���ݸ��ؾ����㷨����һ����ͬ��IP��ַ���أ�����A��¼�����õĶ���������͹���һ����Ⱥ��������ʵ�ָ��ؾ��⡣
DNS�����������ؾ�����ŵ��ǽ����ؾ��⹤������DNS��ʡ�Ե������������鷳��ȱ�����DNS���ܻ���A��¼��������վ���ơ���ʵ�ϣ���
����վ���ǲ���ʹ��DNS������������Ϊ��һ�����ؾ����ֶΣ�Ȼ�������ڲ����ڶ������ؾ��⡣


 2 - ������·�㸺�ؾ���(LVS)
   ������·�㸺�ؾ�����ָ��ͨ��Э���������·���޸�mac��ַ���и��ؾ��⡣
   �������ݴ��䷽ʽ�ֳ������Ǵ���ģʽ�����ؾ������ݷַ������в��޸�IP��ַ��ֻ�޸�Ŀ�ĵ�mac��ַ��ͨ��������ʵ�����������Ⱥ����
 ��������IP�͸��ؾ��������IP��ַһ�����Ӷ��ﵽ���ؾ��⣬���ָ��ؾ��ⷽʽ�ֳ�Ϊֱ��·�ɷ�ʽ��DR��.
   �û����󵽴︺�ؾ���������󣬸��ؾ�����������������ݵ�Ŀ��mac��ַ�޸�Ϊ����WEB��������mac��ַ�������޸����ݰ�Ŀ��IP��ַ��
 ������ݿ�����������Ŀ��WEB���������÷������ڴ��������ݺ���Ծ������ܷ����������Ǹ��ؾ��������ֱ�ӵ����û��������
   ʹ�����Ǵ���ģʽ����·�㸺�ؾ�����Ŀǰ������վ��ʹ�õ�����һ�ָ��ؾ����ֶΡ���linuxƽ̨����õ���·�㸺�ؾ��⿪Դ��Ʒ��
 LVS( Linux Virtual Server )


 3 - IP���ؾ���(SNAT)
   IP���ؾ��⣺���������ͨ���޸�����Ŀ���ַ���и��ؾ��⡣
   �û��������ݰ����︺�ؾ���������󣬸��ؾ���������ڲ���ϵͳ�ں˽��л�ȡ�������ݰ������ݸ��ؾ����㷨����õ�һ̨��ʵ��WEB
 ��������ַ��Ȼ�����ݰ���IP��ַ�޸�Ϊ��ʵ��WEB��������ַ������Ҫͨ���û����̴�����ʵ��WEB������������Ϻ���Ӧ���ݰ���
 �����ؾ�������������ؾ���������ٽ����ݰ�Դ��ַ�޸�Ϊ�����IP��ַ���͸��û��������
   ����Ĺؼ�������ʵWEB��������Ӧ���ݰ���η��ظ����ؾ����������һ���Ǹ��ؾ�����������޸�Ŀ��IP��ַ��ͬʱ�޸�Դ��ַ������
 �ݰ�Դ��ַ��Ϊ�����IP����Դ��ַת����SNAT������һ�ַ����ǽ����ؾ��������ͬʱ��Ϊ��ʵ��������������ط��������������е���
 �ݶ��ᵽ�︺�ؾ��������
   IP���ؾ������ں˽���������ݷַ����Ϸ����������и��õĴ������ܡ�����������������Ӧ�����ݰ�����Ҫ�������ؾ�������������
 ���ؾ�������������Ϊϵͳ��ƿ����


 4 - HTTP�ض����ؾ���(�ټ�)
   HTTP�ض����������һ̨��ͨ��Ӧ�÷���������Ψһ�Ĺ��ܾ��Ǹ����û���HTTP�������һ̨��ʵ�ķ�������ַ��������ʵ�ķ�������ַд
 ��HTTP�ض�����Ӧ�У���Ӧ״̬��302�����ظ��������Ȼ����������Զ�������ʵ�ķ�������
   ���ָ��ؾ��ⷽ�����ŵ��ǱȽϼ򵥣�ȱ�����������Ҫÿ���������η��������������һ�η��ʣ����ܽϲʹ��HTTP302��Ӧ���ض���
 ���������������ж�ΪSEO���ף����������������ض������������Ĵ��������п��ܳ�Ϊƿ����������ַ�����ʵ��ʹ���в������ࡣ


 5 - ��������ؾ���(nginx)
   ��ͳ���������λ�������һ�ˣ������������HTTP�����͵��������ϡ�����������������λ����վ����һ�࣬������վweb����������http����
 �������������Ǳ�����վ��ȫ�����л����������󶼱��뾭��������������൱����web�������Ϳ��ܵ����繥��֮�佨����һ�����ϡ�
   ����֮�⣬���������Ҳ�������û������web���󡣵��û���һ�η��ʾ�̬���ݵ�ʱ�򣬾�̬�ڴ�ͱ������ڷ������������ϣ������������û���
 �ʸþ�̬����ʱ���Ϳ���ֱ�Ӵӷ��������������أ�����web������Ӧ�ٶȣ�����web����������ѹ����
 ���⣬������������Ҳ����ʵ�ָ��ؾ���Ĺ��ܡ�
   ���ڷ�����������ת��������HTTPЭ����棬���Ҳ��Ӧ�ò㸺�ؾ��⡣�ŵ��ǲ���򵥣�ȱ���ǿ��ܳ�Ϊϵͳ��ƿ����
*/
/************************************************************************/

#include "../../util/util.h"

#include <boost/array.hpp>
#include <limits>

class BServer {
public:
	BServer(const std::string ip, int port, int weight = 0) 
		:_ip(ip), 
		_port(port), 
		_weight(weight) {
	}


	std::string _ip;
	int _port;
	int _weight;
	int _id;
};

typedef boost::shared_ptr<BServer> BServerPtr;
typedef std::vector<BServerPtr> BServerPtrArr;

class LoadBalancer {
public:
	LoadBalancer(){}
	~LoadBalancer() {}

	virtual int size() = 0;
	virtual bool empty() = 0;

	virtual BServerPtr get() = 0;
	virtual void add(BServerPtr) = 0;
	virtual void del(BServerPtr) = 0;
};


class RandomAlog : public LoadBalancer {
public:
	RandomAlog() {
		srand(time(NULL));
	}

	~RandomAlog() {
	}

	virtual int size() {
		return _servers_array.size();
	}

	virtual bool empty() {
		return _servers_array.empty();
	}

	virtual BServerPtr get() {
		return _servers_array[rand() % _servers_array.size()];
	}

	virtual void add(BServerPtr ptr) {
		if (!ptr) return;
		//_index.insert(std::make_pair(ptr->_id, _servers_array.size()));
		_servers_array.push_back(ptr);
	}

	virtual void del(BServerPtr ptr) {
		/*boost::unordered_map<int, int>::iterator it = _index.find(ptr->_id);
		if (it != _index.end()) {
			_servers_array.erase(_servers_array.begin() + it->second - 1);
			_index.erase(it);
		}*/

		for (BServerPtrArr::iterator it = _servers_array.begin();
			it != _servers_array.end(); ++it){
			if ((*it)->_id == ptr->_id) {
				_servers_array.erase(it);
			}
		}
	}

private:
	BServerPtrArr _servers_array;
	//boost::unordered_map<int, int> _index;//key: id,value: index(�Ƴ��ڵ��Ӱ�����ڵ������)
};


class RandomWeightAlog : public LoadBalancer {
public:
	RandomWeightAlog() {
		srand(time(NULL));
	}

	~RandomWeightAlog() {}

	virtual int size() {
		return _servers_array.size();
	}

	virtual bool empty() {
		return _servers_array.empty();
	}

	virtual BServerPtr get() {
		return _servers_array[rand() % _servers_array.size()];
	}

	virtual void add(BServerPtr ptr) {
		if (!ptr) return;

		int w = ptr->_weight;
		for(int i = 0; i < w; ++i )
			_servers_array.push_back(ptr);
	}


	virtual void del(BServerPtr ptr) {
		for (BServerPtrArr::iterator it = _servers_array.begin();
			it != _servers_array.end();) {
			if ((*it)->_id == ptr->_id) {
				it = _servers_array.erase(it);
			}else {
				++it;
			}
		}
	}
private:
	BServerPtrArr _servers_array;
};


class RoundRobbinAlgo : public LoadBalancer {
public:
	RoundRobbinAlgo() :_index(0) {}
	~RoundRobbinAlgo() {}

	virtual int size() {
		return _servers_array.size();
	}

	virtual bool empty() {
		return _servers_array.empty();
	}

	virtual BServerPtr get() {
		_index = _index++%_servers_array.size();
		return _servers_array[_index];
	}

	virtual void add(BServerPtr ptr) {
		if (ptr) _servers_array.push_back(ptr);
	}

	virtual void del(BServerPtr ptr) {
		for (BServerPtrArr::iterator it = _servers_array.begin();
			it != _servers_array.end(); ++it) {
			if ((*it)->_id == ptr->_id) {
				_servers_array.erase(it);
			}
		}
	}

private:
	BServerPtrArr _servers_array;
	int _index;
};


class RoundRobbinWeightAlgo : public LoadBalancer {
public:
	RoundRobbinWeightAlgo() :_index(0) {}

	virtual int size() {
		return _servers_array.size();
	}

	virtual bool empty() {
		return _servers_array.empty();
	}

	virtual BServerPtr get() {
		_index = _index++%_servers_array.size();
		return _servers_array[_index];
	}

	virtual void add(BServerPtr ptr) {
		if (!ptr) return;

		int w = ptr->_weight;
		for (int i = 0; i < w; ++i)
			_servers_array.push_back(ptr);
	}

	virtual void del(BServerPtr ptr) {
		for (BServerPtrArr::iterator it = _servers_array.begin();
			it != _servers_array.end();) {
			if ((*it)->_id == ptr->_id) {
				it = _servers_array.erase(it);
			} else {
				++it;
			}
		}
	}

private:
	BServerPtrArr _servers_array;
	int _index;
};


class HashAlgo : public LoadBalancer {
public:
	HashAlgo(boost::function<int(int)> fun):_fun(fun) {}
	~HashAlgo() {}

	virtual int size() {
		return _servers_array.size();
	}

	virtual bool empty() {
		return _servers_array.empty();
	}

	virtual BServerPtr get(int id) {
		if (_fun){
			return _servers_array[_fun(id)];
		}else {
			return _servers_array[hash_algo(id)];
		}
	}

	virtual void add(BServerPtr ptr) {
		if (ptr) _servers_array.push_back(ptr);
	}

	virtual void del(BServerPtr ptr) {
		for (BServerPtrArr::iterator it = _servers_array.begin();
			it != _servers_array.end(); ++it) {
			if ((*it)->_id == ptr->_id) {
				_servers_array.erase(it);
			}
		}
	}

private:
	int hash_algo(int id) {
		return id%_servers_array.size();
	}

	BServerPtrArr _servers_array;
	boost::function<int(int)>	_fun;
};


class HashWeightAlgo : public LoadBalancer {
public:
	HashWeightAlgo(boost::function<int(int)> fun) :_fun(fun) {}
	~HashWeightAlgo() {}


	virtual int size() {
		return _servers_array.size();
	}

	virtual bool empty() {
		return _servers_array.empty();
	}

	virtual BServerPtr get(int id) {
		if (_fun) {
			return _servers_array[_fun(id)];
		} else {
			return _servers_array[hash_algo(id)];
		}
	}

	virtual void add(BServerPtr ptr) {
		if (!ptr) return;

		int w = ptr->_weight;
		for (int i = 0; i < w; ++i)
			_servers_array.push_back(ptr);
	}

	virtual void del(BServerPtr ptr) {
		for (BServerPtrArr::iterator it = _servers_array.begin();
			it != _servers_array.end();) {
			if ((*it)->_id == ptr->_id) {
				it = _servers_array.erase(it);
			} else {
				++it;
			}
		}
	}

private:
	int hash_algo(int id) {
		return id%_servers_array.size();
	}

	BServerPtrArr _servers_array;
	boost::function<int(int)>	_fun;
};

class ConsistentHashAlgo : public LoadBalancer {
	ConsistentHashAlgo(boost::function<int(int)> fun):_fun(fun) {}
	~ConsistentHashAlgo() {}

	virtual int size() {
		return _group.size();
	}

	virtual bool empty() {
		return _group.empty();
	}

	virtual BServerPtr get(int id) {

	}

	virtual void add(BServerPtr ptr) {
		if (!ptr) return;

		_group.insert(std::make_pair(hash_algo(ptr->_id), ptr));
	}

	virtual void del(BServerPtr ptr) {
		_group.erase(hash_algo(ptr->_id));
	}
private:
	int hash_algo(int id) {
		return id%0x7FFFFFFF;
	}

	//0 ~ (2^32) - 1 
	std::map<int, BServerPtr> _group;//key: hash value, value: BServer


	boost::function<int(int)>	_fun;
};


#endif