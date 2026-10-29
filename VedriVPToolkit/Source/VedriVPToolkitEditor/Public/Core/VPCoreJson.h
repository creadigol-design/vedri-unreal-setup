// Copyright vedri studio Ltd. All Rights Reserved.

#pragma once

// Minimal JSON DOM, parser and writer for wizard presets.
//
// Why not the engine's FJsonObject / FJsonSerializer: presets are part of the engine-independent core,
// and the standalone tests on the Mac must exercise exactly the code that reads and writes them on the
// PC. The editor layer only moves bytes (FFileHelper) in and out of these functions.
//
// Scope is deliberately small: UTF-8 in and out, strict RFC 8259 grammar, a UTF-8 BOM is tolerated
// (Notepad adds one), duplicate object keys are rejected, nesting is capped. Numbers are doubles and are
// written locale-independently with the shortest precision that round-trips.

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <locale>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace Vedri::Core
{
	class JsonValue
	{
	public:
		enum class EType : uint8_t
		{
			Null,
			Bool,
			Number,
			String,
			Array,
			Object
		};

		JsonValue() = default;
		JsonValue(std::nullptr_t) {}
		JsonValue(bool bInValue) : Type(EType::Bool), bBoolValue(bInValue) {}
		JsonValue(double InValue) : Type(EType::Number), NumberValue(InValue) {}
		JsonValue(const char* InValue) : Type(EType::String), StringValue(InValue ? InValue : "") {}
		JsonValue(std::string InValue) : Type(EType::String), StringValue(std::move(InValue)) {}

		template <typename T, std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool>, int> = 0>
		JsonValue(T InValue) : Type(EType::Number), NumberValue(static_cast<double>(InValue))
		{
		}

		static JsonValue MakeArray()
		{
			JsonValue Value;
			Value.Type = EType::Array;
			return Value;
		}

		static JsonValue MakeObject()
		{
			JsonValue Value;
			Value.Type = EType::Object;
			return Value;
		}

		EType GetType() const { return Type; }
		bool IsNull() const { return Type == EType::Null; }
		bool IsBool() const { return Type == EType::Bool; }
		bool IsNumber() const { return Type == EType::Number; }
		bool IsString() const { return Type == EType::String; }
		bool IsArray() const { return Type == EType::Array; }
		bool IsObject() const { return Type == EType::Object; }

		bool AsBool(bool bDefault = false) const { return Type == EType::Bool ? bBoolValue : bDefault; }
		double AsNumber(double Default = 0.0) const { return Type == EType::Number ? NumberValue : Default; }

		const std::string& AsString() const
		{
			static const std::string Empty;
			return Type == EType::String ? StringValue : Empty;
		}

		// Arrays

		size_t Size() const { return Type == EType::Array ? Items.size() : (Type == EType::Object ? Keys.size() : 0); }

		const JsonValue& At(size_t Index) const { return Items[Index]; }

		JsonValue& Push(JsonValue Value)
		{
			if (Type != EType::Array)
			{
				*this = MakeArray();
			}
			Items.push_back(std::move(Value));
			return Items.back();
		}

		// Objects (insertion order is preserved; the writer can sort keys for canonical output)

		const std::string& KeyAt(size_t Index) const { return Keys[Index]; }
		const JsonValue& ValueAt(size_t Index) const { return Items[Index]; }

		const JsonValue* Find(const std::string& Key) const
		{
			if (Type != EType::Object)
			{
				return nullptr;
			}
			for (size_t Index = 0; Index < Keys.size(); ++Index)
			{
				if (Keys[Index] == Key)
				{
					return &Items[Index];
				}
			}
			return nullptr;
		}

		/** Sets Key, replacing an existing value in place. Turns a non-object into an empty object first. */
		JsonValue& Set(const std::string& Key, JsonValue Value)
		{
			if (Type != EType::Object)
			{
				*this = MakeObject();
			}
			for (size_t Index = 0; Index < Keys.size(); ++Index)
			{
				if (Keys[Index] == Key)
				{
					Items[Index] = std::move(Value);
					return Items[Index];
				}
			}
			Keys.push_back(Key);
			Items.push_back(std::move(Value));
			return Items.back();
		}

		/** Deep equality. Object member order is ignored; array order is not. */
		bool operator==(const JsonValue& Other) const
		{
			if (Type != Other.Type)
			{
				return false;
			}
			switch (Type)
			{
			case EType::Null:
				return true;
			case EType::Bool:
				return bBoolValue == Other.bBoolValue;
			case EType::Number:
				return NumberValue == Other.NumberValue;
			case EType::String:
				return StringValue == Other.StringValue;
			case EType::Array:
				return Items == Other.Items;
			case EType::Object:
				if (Keys.size() != Other.Keys.size())
				{
					return false;
				}
				for (size_t Index = 0; Index < Keys.size(); ++Index)
				{
					const JsonValue* OtherValue = Other.Find(Keys[Index]);
					if (!OtherValue || !(*OtherValue == Items[Index]))
					{
						return false;
					}
				}
				return true;
			}
			return false;
		}

		bool operator!=(const JsonValue& Other) const { return !(*this == Other); }

	private:
		EType Type = EType::Null;
		bool bBoolValue = false;
		double NumberValue = 0.0;
		std::string StringValue;
		// Array elements, or object values (parallel to Keys). Keeping both in one vector of the
		// enclosing type avoids a container of an incomplete pair type.
		std::vector<JsonValue> Items;
		std::vector<std::string> Keys;
	};

	namespace JsonDetail
	{
		inline bool ParseDoubleClassic(const std::string& Text, double& OutValue)
		{
			std::istringstream Stream(Text);
			Stream.imbue(std::locale::classic());
			double Value = 0.0;
			Stream >> Value;
			if (Stream.fail() || !std::isfinite(Value))
			{
				return false;
			}
			// Must have consumed everything.
			Stream.peek();
			if (!Stream.eof())
			{
				return false;
			}
			OutValue = Value;
			return true;
		}

		inline void AppendUtf8(std::string& Out, uint32_t CodePoint)
		{
			if (CodePoint < 0x80)
			{
				Out.push_back(static_cast<char>(CodePoint));
			}
			else if (CodePoint < 0x800)
			{
				Out.push_back(static_cast<char>(0xC0 | (CodePoint >> 6)));
				Out.push_back(static_cast<char>(0x80 | (CodePoint & 0x3F)));
			}
			else if (CodePoint < 0x10000)
			{
				Out.push_back(static_cast<char>(0xE0 | (CodePoint >> 12)));
				Out.push_back(static_cast<char>(0x80 | ((CodePoint >> 6) & 0x3F)));
				Out.push_back(static_cast<char>(0x80 | (CodePoint & 0x3F)));
			}
			else
			{
				Out.push_back(static_cast<char>(0xF0 | (CodePoint >> 18)));
				Out.push_back(static_cast<char>(0x80 | ((CodePoint >> 12) & 0x3F)));
				Out.push_back(static_cast<char>(0x80 | ((CodePoint >> 6) & 0x3F)));
				Out.push_back(static_cast<char>(0x80 | (CodePoint & 0x3F)));
			}
		}

		/**
		 * Decodes one UTF-8 code point starting at Pos. Returns false on malformed, overlong or surrogate
		 * sequences. Shared with the preset file-name slugger.
		 */
		inline bool DecodeUtf8(const std::string& Text, size_t& Pos, uint32_t& OutCodePoint)
		{
			const unsigned char Lead = static_cast<unsigned char>(Text[Pos]);
			size_t Length = 0;
			uint32_t CodePoint = 0;
			if (Lead < 0x80)
			{
				OutCodePoint = Lead;
				++Pos;
				return true;
			}
			else if ((Lead & 0xE0) == 0xC0)
			{
				Length = 2;
				CodePoint = Lead & 0x1Fu;
			}
			else if ((Lead & 0xF0) == 0xE0)
			{
				Length = 3;
				CodePoint = Lead & 0x0Fu;
			}
			else if ((Lead & 0xF8) == 0xF0)
			{
				Length = 4;
				CodePoint = Lead & 0x07u;
			}
			else
			{
				return false;
			}
			if (Pos + Length > Text.size())
			{
				return false;
			}
			for (size_t Index = 1; Index < Length; ++Index)
			{
				const unsigned char Continuation = static_cast<unsigned char>(Text[Pos + Index]);
				if ((Continuation & 0xC0) != 0x80)
				{
					return false;
				}
				CodePoint = (CodePoint << 6) | (Continuation & 0x3Fu);
			}
			static constexpr uint32_t MinForLength[5] = {0, 0, 0x80, 0x800, 0x10000};
			if (CodePoint < MinForLength[Length] || CodePoint > 0x10FFFF || (CodePoint >= 0xD800 && CodePoint <= 0xDFFF))
			{
				return false;
			}
			OutCodePoint = CodePoint;
			Pos += Length;
			return true;
		}

		class Parser
		{
		public:
			explicit Parser(const std::string& InText) : Text(InText) {}

			bool Run(JsonValue& OutValue, std::string& OutError)
			{
				if (Text.compare(0, 3, "\xEF\xBB\xBF") == 0)
				{
					Pos = 3;
				}
				for (size_t Check = Pos; Check < Text.size();)
				{
					uint32_t Ignored = 0;
					if (!DecodeUtf8(Text, Check, Ignored))
					{
						Pos = Check;
						Fail("invalid UTF-8");
						OutError = Error;
						return false;
					}
				}
				SkipWhitespace();
				JsonValue Value;
				if (!ParseValue(Value))
				{
					OutError = Error;
					return false;
				}
				SkipWhitespace();
				if (Pos != Text.size())
				{
					Fail("unexpected characters after the JSON value");
					OutError = Error;
					return false;
				}
				OutValue = std::move(Value);
				return true;
			}

		private:
			static constexpr int MaxDepth = 64;

			const std::string& Text;
			size_t Pos = 0;
			int Depth = 0;
			std::string Error;

			bool Fail(const char* Message)
			{
				size_t Line = 1;
				size_t Column = 1;
				for (size_t Index = 0; Index < Pos && Index < Text.size(); ++Index)
				{
					if (Text[Index] == '\n')
					{
						++Line;
						Column = 1;
					}
					else
					{
						++Column;
					}
				}
				Error = "line " + std::to_string(Line) + ", column " + std::to_string(Column) + ": " + Message;
				return false;
			}

			bool AtEnd() const { return Pos >= Text.size(); }
			char Peek() const { return AtEnd() ? '\0' : Text[Pos]; }

			void SkipWhitespace()
			{
				while (!AtEnd())
				{
					const char C = Text[Pos];
					if (C == ' ' || C == '\t' || C == '\n' || C == '\r')
					{
						++Pos;
					}
					else
					{
						break;
					}
				}
			}

			bool ParseValue(JsonValue& Out)
			{
				switch (Peek())
				{
				case '{':
					return ParseObject(Out);
				case '[':
					return ParseArray(Out);
				case '"':
				{
					std::string Value;
					if (!ParseString(Value))
					{
						return false;
					}
					Out = JsonValue(std::move(Value));
					return true;
				}
				case 't':
					return ParseLiteral("true", JsonValue(true), Out);
				case 'f':
					return ParseLiteral("false", JsonValue(false), Out);
				case 'n':
					return ParseLiteral("null", JsonValue(), Out);
				default:
					break;
				}
				if (Peek() == '-' || (Peek() >= '0' && Peek() <= '9'))
				{
					return ParseNumber(Out);
				}
				return Fail(AtEnd() ? "unexpected end of input" : "unexpected character");
			}

			bool ParseLiteral(const char* Literal, JsonValue Value, JsonValue& Out)
			{
				const std::string Expected(Literal);
				if (Text.compare(Pos, Expected.size(), Expected) != 0)
				{
					return Fail("invalid literal");
				}
				Pos += Expected.size();
				Out = std::move(Value);
				return true;
			}

			bool ParseNumber(JsonValue& Out)
			{
				const size_t Start = Pos;
				if (Peek() == '-')
				{
					++Pos;
				}
				if (Peek() == '0')
				{
					++Pos;
				}
				else if (Peek() >= '1' && Peek() <= '9')
				{
					while (Peek() >= '0' && Peek() <= '9')
					{
						++Pos;
					}
				}
				else
				{
					return Fail("invalid number");
				}
				if (Peek() == '.')
				{
					++Pos;
					if (!(Peek() >= '0' && Peek() <= '9'))
					{
						return Fail("invalid number: digit expected after '.'");
					}
					while (Peek() >= '0' && Peek() <= '9')
					{
						++Pos;
					}
				}
				if (Peek() == 'e' || Peek() == 'E')
				{
					++Pos;
					if (Peek() == '+' || Peek() == '-')
					{
						++Pos;
					}
					if (!(Peek() >= '0' && Peek() <= '9'))
					{
						return Fail("invalid number: digit expected in exponent");
					}
					while (Peek() >= '0' && Peek() <= '9')
					{
						++Pos;
					}
				}
				double Value = 0.0;
				if (!ParseDoubleClassic(Text.substr(Start, Pos - Start), Value))
				{
					Pos = Start;
					return Fail("number out of range");
				}
				Out = JsonValue(Value);
				return true;
			}

			bool ParseHex4(uint32_t& OutValue)
			{
				if (Pos + 4 > Text.size())
				{
					return Fail("truncated \\u escape");
				}
				uint32_t Value = 0;
				for (int Index = 0; Index < 4; ++Index)
				{
					const char C = Text[Pos++];
					Value <<= 4;
					if (C >= '0' && C <= '9')
					{
						Value |= static_cast<uint32_t>(C - '0');
					}
					else if (C >= 'a' && C <= 'f')
					{
						Value |= static_cast<uint32_t>(C - 'a' + 10);
					}
					else if (C >= 'A' && C <= 'F')
					{
						Value |= static_cast<uint32_t>(C - 'A' + 10);
					}
					else
					{
						return Fail("invalid hex digit in \\u escape");
					}
				}
				OutValue = Value;
				return true;
			}

			bool ParseString(std::string& Out)
			{
				++Pos; // opening quote
				while (true)
				{
					if (AtEnd())
					{
						return Fail("unterminated string");
					}
					const char C = Text[Pos];
					if (C == '"')
					{
						++Pos;
						return true;
					}
					if (static_cast<unsigned char>(C) < 0x20)
					{
						return Fail("control character in string");
					}
					if (C != '\\')
					{
						Out.push_back(C);
						++Pos;
						continue;
					}
					++Pos;
					if (AtEnd())
					{
						return Fail("unterminated escape");
					}
					const char Escape = Text[Pos++];
					switch (Escape)
					{
					case '"':
						Out.push_back('"');
						break;
					case '\\':
						Out.push_back('\\');
						break;
					case '/':
						Out.push_back('/');
						break;
					case 'b':
						Out.push_back('\b');
						break;
					case 'f':
						Out.push_back('\f');
						break;
					case 'n':
						Out.push_back('\n');
						break;
					case 'r':
						Out.push_back('\r');
						break;
					case 't':
						Out.push_back('\t');
						break;
					case 'u':
					{
						uint32_t CodePoint = 0;
						if (!ParseHex4(CodePoint))
						{
							return false;
						}
						if (CodePoint >= 0xDC00 && CodePoint <= 0xDFFF)
						{
							return Fail("unpaired low surrogate");
						}
						if (CodePoint >= 0xD800 && CodePoint <= 0xDBFF)
						{
							if (Text.compare(Pos, 2, "\\u") != 0)
							{
								return Fail("unpaired high surrogate");
							}
							Pos += 2;
							uint32_t Low = 0;
							if (!ParseHex4(Low))
							{
								return false;
							}
							if (Low < 0xDC00 || Low > 0xDFFF)
							{
								return Fail("invalid low surrogate");
							}
							CodePoint = 0x10000 + ((CodePoint - 0xD800) << 10) + (Low - 0xDC00);
						}
						AppendUtf8(Out, CodePoint);
						break;
					}
					default:
						return Fail("invalid escape");
					}
				}
			}

			bool Enter()
			{
				if (++Depth > MaxDepth)
				{
					return Fail("nesting too deep");
				}
				return true;
			}

			bool ParseArray(JsonValue& Out)
			{
				if (!Enter())
				{
					return false;
				}
				++Pos; // [
				JsonValue Array = JsonValue::MakeArray();
				SkipWhitespace();
				if (Peek() == ']')
				{
					++Pos;
					--Depth;
					Out = std::move(Array);
					return true;
				}
				while (true)
				{
					SkipWhitespace();
					JsonValue Element;
					if (!ParseValue(Element))
					{
						return false;
					}
					Array.Push(std::move(Element));
					SkipWhitespace();
					if (Peek() == ',')
					{
						++Pos;
						continue;
					}
					if (Peek() == ']')
					{
						++Pos;
						break;
					}
					return Fail("expected ',' or ']'");
				}
				--Depth;
				Out = std::move(Array);
				return true;
			}

			bool ParseObject(JsonValue& Out)
			{
				if (!Enter())
				{
					return false;
				}
				++Pos; // {
				JsonValue Object = JsonValue::MakeObject();
				SkipWhitespace();
				if (Peek() == '}')
				{
					++Pos;
					--Depth;
					Out = std::move(Object);
					return true;
				}
				while (true)
				{
					SkipWhitespace();
					if (Peek() != '"')
					{
						return Fail("expected a string key");
					}
					const size_t KeyPos = Pos;
					std::string Key;
					if (!ParseString(Key))
					{
						return false;
					}
					if (Object.Find(Key))
					{
						Pos = KeyPos;
						return Fail(("duplicate key \"" + Key + "\"").c_str());
					}
					SkipWhitespace();
					if (Peek() != ':')
					{
						return Fail("expected ':'");
					}
					++Pos;
					SkipWhitespace();
					JsonValue Member;
					if (!ParseValue(Member))
					{
						return false;
					}
					Object.Set(Key, std::move(Member));
					SkipWhitespace();
					if (Peek() == ',')
					{
						++Pos;
						continue;
					}
					if (Peek() == '}')
					{
						++Pos;
						break;
					}
					return Fail("expected ',' or '}'");
				}
				--Depth;
				Out = std::move(Object);
				return true;
			}
		};

		inline void AppendEscapedString(std::string& Out, const std::string& Value)
		{
			static constexpr char HexDigits[] = "0123456789abcdef";
			Out.push_back('"');
			for (const char C : Value)
			{
				switch (C)
				{
				case '"':
					Out += "\\\"";
					break;
				case '\\':
					Out += "\\\\";
					break;
				case '\b':
					Out += "\\b";
					break;
				case '\f':
					Out += "\\f";
					break;
				case '\n':
					Out += "\\n";
					break;
				case '\r':
					Out += "\\r";
					break;
				case '\t':
					Out += "\\t";
					break;
				default:
					if (static_cast<unsigned char>(C) < 0x20)
					{
						const unsigned char Byte = static_cast<unsigned char>(C);
						Out += "\\u00";
						Out.push_back(HexDigits[Byte >> 4]);
						Out.push_back(HexDigits[Byte & 0x0F]);
					}
					else
					{
						Out.push_back(C);
					}
					break;
				}
			}
			Out.push_back('"');
		}
	}

	/** Shortest locale-independent representation that parses back to the same double. */
	inline std::string FormatJsonNumber(double Value)
	{
		if (!std::isfinite(Value))
		{
			return "null";
		}
		if (Value == 0.0)
		{
			return "0";
		}
		double IntegralPart = 0.0;
		if (std::modf(Value, &IntegralPart) == 0.0 && std::fabs(Value) < 9007199254740992.0)
		{
			return std::to_string(static_cast<long long>(Value));
		}
		for (const int Precision : {15, 16, 17})
		{
			std::ostringstream Stream;
			Stream.imbue(std::locale::classic());
			Stream.precision(Precision);
			Stream << Value;
			const std::string Text = Stream.str();
			double Parsed = 0.0;
			if (Precision == 17 || (JsonDetail::ParseDoubleClassic(Text, Parsed) && Parsed == Value))
			{
				return Text;
			}
		}
		return "null";
	}

	struct JsonWriteOptions
	{
		bool bPretty = true;
		bool bSortKeys = false;
	};

	namespace JsonDetail
	{
		inline void WriteValue(std::string& Out, const JsonValue& Value, const JsonWriteOptions& Options, int Indent)
		{
			const auto NewLine = [&Out, &Options](int Level)
			{
				if (Options.bPretty)
				{
					Out.push_back('\n');
					Out.append(static_cast<size_t>(Level) * 2, ' ');
				}
			};

			switch (Value.GetType())
			{
			case JsonValue::EType::Null:
				Out += "null";
				break;
			case JsonValue::EType::Bool:
				Out += Value.AsBool() ? "true" : "false";
				break;
			case JsonValue::EType::Number:
				Out += FormatJsonNumber(Value.AsNumber());
				break;
			case JsonValue::EType::String:
				AppendEscapedString(Out, Value.AsString());
				break;
			case JsonValue::EType::Array:
				if (Value.Size() == 0)
				{
					Out += "[]";
					break;
				}
				Out.push_back('[');
				for (size_t Index = 0; Index < Value.Size(); ++Index)
				{
					if (Index > 0)
					{
						Out.push_back(',');
					}
					NewLine(Indent + 1);
					WriteValue(Out, Value.At(Index), Options, Indent + 1);
				}
				NewLine(Indent);
				Out.push_back(']');
				break;
			case JsonValue::EType::Object:
			{
				if (Value.Size() == 0)
				{
					Out += "{}";
					break;
				}
				std::vector<size_t> Order(Value.Size());
				for (size_t Index = 0; Index < Order.size(); ++Index)
				{
					Order[Index] = Index;
				}
				if (Options.bSortKeys)
				{
					std::sort(Order.begin(), Order.end(), [&Value](size_t A, size_t B) { return Value.KeyAt(A) < Value.KeyAt(B); });
				}
				Out.push_back('{');
				for (size_t Index = 0; Index < Order.size(); ++Index)
				{
					if (Index > 0)
					{
						Out.push_back(',');
					}
					NewLine(Indent + 1);
					AppendEscapedString(Out, Value.KeyAt(Order[Index]));
					Out += Options.bPretty ? ": " : ":";
					WriteValue(Out, Value.ValueAt(Order[Index]), Options, Indent + 1);
				}
				NewLine(Indent);
				Out.push_back('}');
				break;
			}
			}
		}
	}

	/** Parses UTF-8 JSON text. On failure returns false and OutError holds "line L, column C: reason". */
	inline bool ParseJson(const std::string& Text, JsonValue& OutValue, std::string& OutError)
	{
		JsonDetail::Parser Parser(Text);
		return Parser.Run(OutValue, OutError);
	}

	/** Serialises to UTF-8 JSON. Pretty output uses two-space indentation and ends with a newline. */
	inline std::string WriteJson(const JsonValue& Value, const JsonWriteOptions& Options = JsonWriteOptions())
	{
		std::string Out;
		JsonDetail::WriteValue(Out, Value, Options, 0);
		if (Options.bPretty)
		{
			Out.push_back('\n');
		}
		return Out;
	}

	/** Compact output with sorted keys: equal values always produce identical bytes. */
	inline std::string WriteCanonicalJson(const JsonValue& Value)
	{
		JsonWriteOptions Options;
		Options.bPretty = false;
		Options.bSortKeys = true;
		return WriteJson(Value, Options);
	}

	/** 64-bit FNV-1a; used for generator fingerprints, not for security. */
	inline uint64_t Fnv1a64(const std::string& Bytes)
	{
		uint64_t Hash = 14695981039346656037ull;
		for (const char C : Bytes)
		{
			Hash ^= static_cast<uint64_t>(static_cast<unsigned char>(C));
			Hash *= 1099511628211ull;
		}
		return Hash;
	}

	inline std::string ToHex64(uint64_t Value)
	{
		static constexpr char HexDigits[] = "0123456789abcdef";
		std::string Text(16, '0');
		for (int Index = 15; Index >= 0; --Index)
		{
			Text[static_cast<size_t>(Index)] = HexDigits[Value & 0xF];
			Value >>= 4;
		}
		return Text;
	}

	/** Hex FNV-1a of the canonical form: the generator fingerprint stored on generated assets and files. */
	inline std::string FingerprintJson(const JsonValue& Value)
	{
		return ToHex64(Fnv1a64(WriteCanonicalJson(Value)));
	}
}
