//�������������쳣���ݸ�future����future::get()ʱ���������ִ�й����е��쳣

#include <iostream>
#include <string>

#include "task.h"
#include "task_context.h"
#include "threadpool_context.h"

using namespace std::literals;

void test_task_exception()
{
	using namespace st;

	auto t = make_task([](int val)
	{
		std::cout << "delay run " << std::this_thread::get_id() << std::endl;
		if (val == 0) throw std::exception("divide by zero");
		return 10 / val;
	}).then(async_context, [](int val)
	{
		std::this_thread::sleep_for(1s);
		std::cout << "run in another thread " << std::this_thread::get_id() << std::endl;
		if (val == 0) throw std::exception("divide by zero");
		return 10 / val;
	});

	t(0);		//5 : ����; 20 : then()��Ĵ������; 0 : make_task()��Ĵ������

	auto f = t.get_future();
	try
	{
		std::cout << "end value is " << f.get() << std::endl;
	}
	catch (std::exception ex)
	{
		std::cout << ex.what() << std::endl;
	}
	catch (...)
	{
		std::cout << "had some exception!" << std::endl;
	}
}
