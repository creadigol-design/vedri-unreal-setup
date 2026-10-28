// Copyright vedri studio Ltd. All Rights Reserved.

// A deliberately tiny test harness (no third-party framework, nothing to licence or vendor).
// Built with -fno-exceptions -fno-rtti to match Unreal's defaults for the code under test.

#pragma once

#include <cmath>
#include <cstdio>
#include <string>
#include <type_traits>
#include <vector>

namespace VedriTest
{
	struct TestCase
	{
		const char* Name;
		void (*Function)();
	};

	inline std::vector<TestCase>& Registry()
	{
		static std::vector<TestCase> Tests;
		return Tests;
	}

	inline int& FailureCount()
	{
		static int Count = 0;
		return Count;
	}

	inline const char*& CurrentTestName()
	{
		static const char* Name = "";
		return Name;
	}

	struct Registrar
	{
		Registrar(const char* Name, void (*Function)()) { Registry().push_back(TestCase{Name, Function}); }
	};

	inline void ReportFailure(const char* File, int Line, const std::string& Message)
	{
		++FailureCount();
		std::fprintf(stderr, "%s:%d: FAILED [%s] %s\n", File, Line, CurrentTestName(), Message.c_str());
	}

	template <typename T>
	std::string Describe(const T& Value)
	{
		if constexpr (std::is_same_v<T, bool>)
		{
			return Value ? "true" : "false";
		}
		else if constexpr (std::is_enum_v<T>)
		{
			return "enum(" + std::to_string(static_cast<long long>(Value)) + ")";
		}
		else if constexpr (std::is_arithmetic_v<T>)
		{
			return std::to_string(Value);
		}
		else if constexpr (std::is_convertible_v<const T&, std::string>)
		{
			return "\"" + std::string(Value) + "\"";
		}
		else
		{
			return "<value>";
		}
	}
}

#define VEDRI_TEST(TestName)                                                              \
	static void TestName();                                                               \
	static const VedriTest::Registrar TestName##Registrar(#TestName, &TestName);          \
	static void TestName()

#define EXPECT_TRUE(Expr)                                                                 \
	do                                                                                    \
	{                                                                                     \
		if (!(Expr))                                                                      \
		{                                                                                 \
			VedriTest::ReportFailure(__FILE__, __LINE__, "expected true: " #Expr);         \
		}                                                                                 \
	} while (0)

#define EXPECT_FALSE(Expr) EXPECT_TRUE(!(Expr))

#define EXPECT_EQ(Actual, Expected)                                                       \
	do                                                                                    \
	{                                                                                     \
		const auto VedriActual = (Actual);                                               \
		const auto VedriExpected = (Expected);                                           \
		if (!(VedriActual == VedriExpected))                                              \
		{                                                                                 \
			VedriTest::ReportFailure(__FILE__, __LINE__,                                  \
				"expected " #Actual " == " #Expected ", got " + VedriTest::Describe(VedriActual) + \
				" vs " + VedriTest::Describe(VedriExpected));                             \
		}                                                                                 \
	} while (0)

#define EXPECT_NEAR(Actual, Expected, Tolerance)                                          \
	do                                                                                    \
	{                                                                                     \
		const double VedriActual = static_cast<double>(Actual);                           \
		const double VedriExpected = static_cast<double>(Expected);                       \
		if (!(std::fabs(VedriActual - VedriExpected) <= (Tolerance)))                     \
		{                                                                                 \
			VedriTest::ReportFailure(__FILE__, __LINE__,                                  \
				"expected " #Actual " ~= " #Expected ", got " + std::to_string(VedriActual) + \
				" vs " + std::to_string(VedriExpected));                                  \
		}                                                                                 \
	} while (0)

/** Like EXPECT_TRUE but stops the current test, for preconditions later checks depend on. */
#define ASSERT_TRUE(Expr)                                                                 \
	do                                                                                    \
	{                                                                                     \
		if (!(Expr))                                                                      \
		{                                                                                 \
			VedriTest::ReportFailure(__FILE__, __LINE__, "assertion failed: " #Expr);      \
			return;                                                                       \
		}                                                                                 \
	} while (0)
