//���Խ��ص��ӿڵĺ�������װ���������ڵ�
#include <iostream>
#include <string>

#include "task.h"
#include "task_context.h"
#include "threadpool_context.h"

using namespace std::literals;

//foo_callback ��Ҫ��������Ҫ��
//һ������ֵû������
//����cb��Ȼ�ᱻ���ã���ֻ����һ�Ρ������ڲ������쳣
//����cbû�з���ֵ
void foo_callback(int val, const std::function<void(int, std::string)> & cb, std::string str)
{
	std::cout << "foo_callback: " << val << " @" << std::this_thread::get_id() << std::endl;
	cb(val * 2, str);
}

void test_task_marshal()
{
	using namespace st;

	threadpool_context pool_context{ 1 };

	auto t = marshal_task(&foo_callback, 1, st::_cb, "first run "s)
		.marshal(pool_context, &foo_callback, std::placeholders::_2, st::_cb, std::placeholders::_3)
		.then(async_context, [](int val, std::string str)
		{
			std::cout << str << val << "@" << std::this_thread::get_id() << std::endl;
			return val * 2;
		})
		;

	t();		//��ʼ����������

	auto f = t.get_future();
	auto val = f.get();
	std::cout << "end value is " << val << "@" << std::this_thread::get_id() << std::endl;
}
