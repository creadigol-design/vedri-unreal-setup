// Copyright vedri studio Ltd. All Rights Reserved.

#include "TestHarness.h"

#include "Core/VPCoreJson.h"

#include <clocale>
#include <string>
#include <vector>

using namespace Vedri::Core;

namespace
{
	JsonValue ParseOk(const std::string& Text)
	{
		JsonValue Value;
		std::string Error;
		if (!ParseJson(Text, Value, Error))
		{
			VedriTest::ReportFailure(__FILE__, __LINE__, "unexpected parse failure for " + Text + ": " + Error);
		}
		return Value;
	}

	bool ParseFails(const std::string& Text)
	{
		JsonValue Value;
		std::string Error;
		return !ParseJson(Text, Value, Error) && !Error.empty();
	}
}

VEDRI_TEST(Json_ParsesScalarsAndNesting)
{
	const JsonValue Value = ParseOk(R"({"a": 1, "b": [true, false, null, "x"], "c": {"d": -2.5e1}})");
	ASSERT_TRUE(Value.IsObject());
	EXPECT_EQ(Value.Size(), size_t(3));
	EXPECT_EQ(Value.Find("a")->AsNumber(), 1.0);
	const JsonValue* B = Value.Find("b");
	ASSERT_TRUE(B && B->IsArray());
	EXPECT_EQ(B->Size(), size_t(4));
	EXPECT_TRUE(B->At(0).AsBool());
	EXPECT_FALSE(B->At(1).AsBool(true));
	EXPECT_TRUE(B->At(2).IsNull());
	EXPECT_EQ(B->At(3).AsString(), "x");
	EXPECT_EQ(Value.Find("c")->Find("d")->AsNumber(), -25.0);
	EXPECT_TRUE(Value.Find("missing") == nullptr);
}

VEDRI_TEST(Json_PrettyOutputRoundTripsExactly)
{
	const std::string Text =
		"{\n"
		"  \"name\": \"vedr\xC3\xAD\",\n"
		"  \"list\": [\n"
		"    1,\n"
		"    2.5\n"
		"  ],\n"
		"  \"empty\": {},\n"
		"  \"none\": []\n"
		"}\n";
	EXPECT_EQ(WriteJson(ParseOk(Text)), Text);
}

VEDRI_TEST(Json_StringEscapesAndUnicode)
{
	EXPECT_EQ(ParseOk(R"("\u00ed")").AsString(), "\xC3\xAD");
	EXPECT_EQ(ParseOk(R"("\u0175")").AsString(), "\xC5\xB5"); // Welsh w-circumflex
	EXPECT_EQ(ParseOk(R"("\uD83C\uDFAC")").AsString(), "\xF0\x9F\x8E\xAC"); // surrogate pair
	EXPECT_EQ(ParseOk(R"("a\"b\\c\/d\n\t")").AsString(), "a\"b\\c/d\n\t");
	EXPECT_EQ(ParseOk("\"Ll\xC5\xB7n\"").AsString(), "Ll\xC5\xB7n"); // raw UTF-8 passes through

	JsonValue Value("tab\there \"quoted\" \x01 back\\slash");
	EXPECT_EQ(WriteJson(Value, JsonWriteOptions{false, false}), R"("tab\there \"quoted\" \u0001 back\\slash")");
}

VEDRI_TEST(Json_RejectsInvalidInput)
{
	const std::vector<std::string> Bad = {
		"",
		"{",
		"[1,]",
		"{\"a\":1,}",
		"01",
		"1.",
		"-",
		"1e",
		"tru",
		"1 2",
		"{a:1}",
		"\"\\x\"",
		"\"line\nbreak\"",
		"{\"a\":1,\"a\":2}",
		"\"\\uDC00\"",
		"\"\\uD800\"",
		"\"\\uD800\\u0041\"",
		"\"\\u12G4\"",
		"\"\xC3\x28\"",     // invalid continuation byte
		"\"\xC0\xAF\"",     // overlong encoding
		"\"\xED\xA0\x80\"", // UTF-8 encoded surrogate
		"1e999",
	};
	for (const std::string& Text : Bad)
	{
		if (!ParseFails(Text))
		{
			VedriTest::ReportFailure(__FILE__, __LINE__, "expected parse failure for: " + Text);
		}
	}
}

VEDRI_TEST(Json_NestingIsCapped)
{
	EXPECT_FALSE(ParseFails(std::string(64, '[') + std::string(64, ']')));
	EXPECT_TRUE(ParseFails(std::string(65, '[') + std::string(65, ']')));
}

VEDRI_TEST(Json_ErrorReportsLineAndColumn)
{
	JsonValue Value;
	std::string Error;
	EXPECT_FALSE(ParseJson("{\n  \"a\": tru\n}", Value, Error));
	EXPECT_EQ(Error.rfind("line 2, column 8:", 0), size_t(0));
}

VEDRI_TEST(Json_AcceptsUtf8Bom)
{
	EXPECT_EQ(ParseOk("\xEF\xBB\xBF{\"a\": 1}").Find("a")->AsNumber(), 1.0);
}

VEDRI_TEST(Json_NumberFormatting)
{
	EXPECT_EQ(FormatJsonNumber(25.0), "25");
	EXPECT_EQ(FormatJsonNumber(-3.0), "-3");
	EXPECT_EQ(FormatJsonNumber(-0.0), "0");
	EXPECT_EQ(FormatJsonNumber(2.84), "2.84");
	EXPECT_EQ(FormatJsonNumber(23.976), "23.976");
	EXPECT_EQ(FormatJsonNumber(0.1), "0.1");
	EXPECT_EQ(FormatJsonNumber(40000.0), "40000");
	for (const double Value : {1.0 / 3.0, 2.0 / 3.0, 1e300, -1e-300, 123456789.123456789, 0.30000000000000004})
	{
		double Parsed = 0.0;
		EXPECT_TRUE(JsonDetail::ParseDoubleClassic(FormatJsonNumber(Value), Parsed));
		EXPECT_EQ(Parsed, Value);
	}
}

VEDRI_TEST(Json_NumbersIgnoreProcessLocale)
{
	// A comma-decimal C locale must not change how presets read or write numbers. Skipped quietly
	// when the machine has no such locale installed.
	const char* Previous = std::setlocale(LC_ALL, nullptr);
	const std::string Saved = Previous ? Previous : "C";
	if (std::setlocale(LC_ALL, "de_DE.UTF-8") || std::setlocale(LC_ALL, "cy_GB.UTF-8") || std::setlocale(LC_ALL, "fr_FR.UTF-8"))
	{
		EXPECT_EQ(FormatJsonNumber(2.5), "2.5");
		EXPECT_EQ(ParseOk("2.5").AsNumber(), 2.5);
	}
	std::setlocale(LC_ALL, Saved.c_str());
}

VEDRI_TEST(Json_ObjectEqualityIgnoresOrderButArraysDoNot)
{
	EXPECT_TRUE(ParseOk(R"({"a":1,"b":[1,2]})") == ParseOk(R"({"b":[1,2],"a":1})"));
	EXPECT_FALSE(ParseOk(R"({"a":1,"b":[1,2]})") == ParseOk(R"({"a":1,"b":[2,1]})"));
	EXPECT_FALSE(ParseOk(R"({"a":1})") == ParseOk(R"({"a":1,"b":2})"));
}

VEDRI_TEST(Json_SetReplacesInPlace)
{
	JsonValue Object = JsonValue::MakeObject();
	Object.Set("first", 1);
	Object.Set("second", 2);
	Object.Set("first", "replaced");
	EXPECT_EQ(Object.Size(), size_t(2));
	EXPECT_EQ(Object.KeyAt(0), "first");
	EXPECT_EQ(Object.ValueAt(0).AsString(), "replaced");
}

VEDRI_TEST(Json_CanonicalFormAndFingerprint)
{
	const JsonValue A = ParseOk(R"({"b":1,"a":{"y":true,"x":null}})");
	const JsonValue B = ParseOk(R"({"a":{"x":null,"y":true},"b":1})");
	EXPECT_EQ(WriteCanonicalJson(A), R"({"a":{"x":null,"y":true},"b":1})");
	EXPECT_EQ(FingerprintJson(A), FingerprintJson(B));
	EXPECT_FALSE(FingerprintJson(A) == FingerprintJson(ParseOk(R"({"b":2,"a":{"y":true,"x":null}})")));

	// Published FNV-1a 64 test vectors.
	EXPECT_EQ(ToHex64(Fnv1a64("")), "cbf29ce484222325");
	EXPECT_EQ(ToHex64(Fnv1a64("a")), "af63dc4c8601ec8c");
	EXPECT_EQ(ToHex64(Fnv1a64("foobar")), "85944171f73967e8");
}
