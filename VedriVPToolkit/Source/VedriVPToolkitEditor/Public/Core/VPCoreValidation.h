// Copyright vedri studio Ltd. All Rights Reserved.

#pragma once

// Engine-independent core (see CLAUDE.md, "Core layer rules"): no UObject, no engine headers,
// no exceptions, no RTTI. Compiled by UBT inside the editor module and by Tests/Standalone.

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <locale>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace Vedri::Core
{
	enum class ESeverity : uint8_t
	{
		Info,
		Warning,
		Error
	};

	/**
	 * A validation finding. Core never produces user-facing text: Code is a stable identifier that the
	 * editor maps to a LOCTEXT message, and Args are the positional arguments ({0}, {1}, ...) for it.
	 * The full list of codes lives in docs/VALIDATION_CODES.md so the en/cy strings can be reviewed.
	 */
	struct ValidationIssue
	{
		ESeverity Severity = ESeverity::Error;
		std::string Code;
		std::string Field;
		std::vector<std::string> Args;

		bool operator==(const ValidationIssue& Other) const = default;
	};

	using IssueList = std::vector<ValidationIssue>;

	inline void AddIssue(IssueList& Issues, ESeverity Severity, std::string Code, std::string Field, std::vector<std::string> Args = {})
	{
		Issues.push_back(ValidationIssue{Severity, std::move(Code), std::move(Field), std::move(Args)});
	}

	inline size_t CountIssues(const IssueList& Issues, ESeverity Severity)
	{
		size_t Count = 0;
		for (const ValidationIssue& Issue : Issues)
		{
			if (Issue.Severity == Severity)
			{
				++Count;
			}
		}
		return Count;
	}

	inline bool HasErrors(const IssueList& Issues)
	{
		return CountIssues(Issues, ESeverity::Error) > 0;
	}

	inline const ValidationIssue* FindIssue(const IssueList& Issues, const std::string& Code)
	{
		for (const ValidationIssue& Issue : Issues)
		{
			if (Issue.Code == Code)
			{
				return &Issue;
			}
		}
		return nullptr;
	}

	/** Prefixes every issue's Field, used when a sub-validator's issues are merged into a parent list. */
	inline void AppendIssues(IssueList& Into, const IssueList& From, const std::string& FieldPrefix)
	{
		for (const ValidationIssue& Issue : From)
		{
			ValidationIssue Copy = Issue;
			Copy.Field = Copy.Field.empty() ? FieldPrefix : FieldPrefix + "." + Copy.Field;
			Into.push_back(std::move(Copy));
		}
	}

	/** Locale-independent fixed-point formatting with trailing zeros trimmed: 3.0 -> "3", 2.60 -> "2.6". */
	inline std::string FormatNumber(double Value, int MaxDecimals = 3)
	{
		if (!std::isfinite(Value))
		{
			return "0";
		}
		std::ostringstream Stream;
		Stream.imbue(std::locale::classic());
		Stream.setf(std::ios::fixed, std::ios::floatfield);
		Stream.precision(MaxDecimals);
		Stream << Value;
		std::string Text = Stream.str();
		if (Text.find('.') != std::string::npos)
		{
			while (!Text.empty() && Text.back() == '0')
			{
				Text.pop_back();
			}
			if (!Text.empty() && Text.back() == '.')
			{
				Text.pop_back();
			}
		}
		if (Text == "-0")
		{
			Text = "0";
		}
		return Text;
	}

	inline std::string FormatUInt(uint64_t Value)
	{
		return std::to_string(Value);
	}
}
