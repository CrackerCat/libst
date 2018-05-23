//����get_future() �� then() ֮��ĳ�ͻ��������ʱ���쳣

#include <iostream>
#include <string>

#include "task.h"
#include "task_context.h"

using namespace std::literals;

void test_task_conflict()
{
	using namespace st;

	auto t = make_task([]
	{
		std::cout << "delay run " << std::this_thread::get_id() << std::endl;
		return 1;
	});

	auto f = t.get_future();		//�������then()�ˡ�

	//����then(),�����쳣
	t.then(async_context, [](int val)
	{
		std::this_thread::sleep_for(5s);
		std::cout << "run in another thread " << std::this_thread::get_id() << std::endl;
		return val;
	});

	t();

#if 1
	std::cout << "end value is " << f.get() << std::endl;
#else
	//Ҳ���Բ�ȡfuture�������ȴ�������Ȼ����Ȼ����
	std::cout << "press any key to continue." << std::endl;
	_getch();
#endif
}
