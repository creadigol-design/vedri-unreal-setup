// Copyright vedri studio Ltd. All Rights Reserved.

#include "TestHarness.h"

#include <cstdio>
#include <cstring>

int main(int ArgCount, char** Args)
{
	// Optional filter: run only tests whose name contains the first argument.
	const char* Filter = ArgCount > 1 ? Args[1] : nullptr;
	int Run = 0;
	for (const VedriTest::TestCase& Test : VedriTest::Registry())
	{
		if (Filter && !std::strstr(Test.Name, Filter))
		{
			continue;
		}
		VedriTest::CurrentTestName() = Test.Name;
		const int FailuresBefore = VedriTest::FailureCount();
		Test.Function();
		std::printf("%s %s\n", VedriTest::FailureCount() == FailuresBefore ? "[ ok ]" : "[FAIL]", Test.Name);
		++Run;
	}
	std::printf("\n%d tests, %d failed checks\n", Run, VedriTest::FailureCount());
	return (VedriTest::FailureCount() == 0 && Run > 0) ? 0 : 1;
}
