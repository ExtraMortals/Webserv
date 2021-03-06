#include "Regex_bonus.hpp"

using namespace regex;

#define ERROR_THROW(x) throw x 

int Regex::matchMetaChar(unsigned char c, unsigned char mc) {
	switch (mc) {
		case 'd':
			return MATCH_DIGIT(c);
		case 'D':
			return !MATCH_DIGIT(c);
		case 'w':
			return MATCH_ALPHANUM(c);
		case 'W':
			return !MATCH_ALPHANUM(c);
		case 's':
			return MATCH_WHITESPACE(c);
		case 'S':
			return !MATCH_WHITESPACE(c);
		default:
			return (c == mc);
	}
}

int Regex::matchCharClass(unsigned char c, const char *str) {
	char max;
	while (*str != '\0') {
		if (str[0] == '\\') {
			if (this->matchMetaChar(c, str[1]))
				return 1;
			str += 2;
			if (IS_METACHAR(*str))
				continue;
		} else {
			if (c == *str)
				return 1;
			str++;
		}
		if (*str != '-' || !str[1])
			continue;
		if ((max = static_cast<char>(str[1] == '\\')) && IS_METACHAR(str[2]))
			continue;
		if (c >= str[-1] && c <= (max ? str[2] : str[1]))
			return 1;
		str++;
	}
	return 0;
}

int Regex::matchOne(RegexNode *node, unsigned char c) {
	switch (node->type) {
		case CHAR:
			return (node->u.ch == c);
		case DOT:
			return MATCH_DOT(c);
		case CHAR_CLASS:
			return this->matchCharClass(c, reinterpret_cast<const char *>(node->u.cc));
		case INV_CHAR_CLASS:
			return !this->matchCharClass(c, reinterpret_cast<const char *>(node->u.cc));
		case DIGIT:
			return MATCH_DIGIT(c);
		case NOT_DIGIT:
			return !MATCH_DIGIT(c);
		case ALPHA:
			return MATCH_ALPHANUM(c);
		case NOT_ALPHA:
			return !MATCH_ALPHANUM(c);
		case WHITESPACE:
			return MATCH_WHITESPACE(c);
		case NOT_WHITESPACE:
			return !MATCH_WHITESPACE(c);
		default:
			return 0;
	}
}

const char *Regex::matchQuantity(RegexNode *nodes, const char *text, const char *textEnd, unsigned min, unsigned max) {
	const char *end, *start = text + min;
	while (max && *text != '\0' && this->matchOne(nodes, *text)) {
		text++;
		max--;
	}

	while (text >= start)
		if ((end = this->matchPattern(nodes + 2, text--, textEnd)))
			return end;
	return NULL;
}

const char *Regex::matchQuantityLazy(RegexNode *nodes, const char *text, const char *textEnd, unsigned min, unsigned max) {
	const char *end;
	max = max - min;

	while (min && *text != '\0' && this->matchOne(nodes, *text)) {
		text++;
		min--;
	}

	if (min)
		return NULL;
	if ((end = this->matchPattern(nodes + 2, text, textEnd)))
		return end;

	while (max && text[0] != '\0' && this->matchOne(nodes, *text)) {
		text++;
		max--;
		if ((end = this->matchPattern(nodes + 2, text, textEnd)))
			return end;
	}
	return NULL;
}

const char *Regex::matchPattern(RegexNode *nodes, const char *text, const char *textEnd) {
	do {
		if (nodes[0].type == NONE)
			return text;
		if ((nodes[0].type == END) && nodes[1].type == NONE)
			return (text == textEnd) ? text : NULL;

		switch (nodes[1].type) {
			case QUESTIONMARK:
				return this->matchQuantity(nodes, text, textEnd, 0, 1);
			case QUESTIONMARK_LAZY:
				return this->matchQuantityLazy(nodes, text, textEnd, 0, 1);
			case QUANTIFIER:
				return this->matchQuantity(nodes, text, textEnd, nodes[1].u.mn[0], nodes[1].u.mn[1]);
			case QUANTIFIER_LAZY:
				return this->matchQuantityLazy(nodes, text, textEnd, nodes[1].u.mn[0], nodes[1].u.mn[1]);
			case STAR:
				return this->matchQuantity(nodes, text, textEnd, 0, MAX_PLUS_STAR);
			case STAR_LAZY:
				return this->matchQuantityLazy(nodes, text, textEnd, 0, MAX_PLUS_STAR);
			case PLUS:
				return this->matchQuantity(nodes, text, textEnd, 1, MAX_PLUS_STAR);
			case PLUS_LAZY:
				return this->matchQuantityLazy(nodes, text, textEnd, 1, MAX_PLUS_STAR);
		}
	} while (text < textEnd && this->matchOne(nodes++, *text++));
	return NULL;
}

bool Regex::match(const std::string &text) {
	if (text.empty())
		ERROR_THROW(NoTextToMatch());

	const char *textStart = text.c_str();
	const char *textEnd = text.c_str() + text.length();
	const char *mend;

	RegexNode *nodes = this->_regex.regexNodes;
	if (nodes[0].type == BEGIN) {
		if ((mend = this->matchPattern(nodes + 1, text.c_str(), textEnd))) {
			if (mend == textEnd && text == textStart)
				return true;
		}
		return false;
	}

	std::string::const_iterator it = text.begin();

	do {
		if ((mend = this->matchPattern(nodes, text.c_str(), textEnd))) {
			if (mend == textEnd && text == textStart) {
				return true;
			}
		}
	} while (it++ != text.end());
	return false;
}

void Regex::compile(const std::string &pattern) {
	if (pattern.empty())
		ERROR_THROW(ImpossiblePattern());

	RegexNode *nodes = this->_regex.regexNodes;
	unsigned char *buffer = this->_regex.regexBuffer;
	size_t bufferLength = sizeof(this->_regex.regexBuffer);
	int i = 0, j = 0, index = 0;
	char quantifiable = '\0';
	unsigned long value;

	while (pattern[i] != '\0' && (j + 1 < MAX_REGEXP_SYMBOLS)) {
		switch (pattern[i]) {
			case '^':
				quantifiable = 0;
				nodes[j].type = BEGIN;
				break;
			case '$':
				quantifiable = 0;
				nodes[j].type = END;
				break;
			case '.':
				quantifiable = 1;
				nodes[j].type = DOT;
				break;
			case '*':
				if (!quantifiable)
					ERROR_THROW(ImpossiblePattern());
				quantifiable = 0;
				if (pattern[i + 1] == '?') {
					i++;
					nodes[j].type = STAR_LAZY;
				} else
					nodes[j].type = STAR;
				break;
			case '+':
				if (!quantifiable)
					ERROR_THROW(ImpossiblePattern());
				quantifiable = 0;
				if (pattern[i + 1] == '?') {
					i++;
					nodes[j].type = PLUS_LAZY;
				} else
					nodes[j].type = PLUS;
				break;
			case '?':
				if (!quantifiable)
					ERROR_THROW(ImpossiblePattern());
				quantifiable = 0;
				if (pattern[i + 1] == '?') {
					i++;
					nodes[j].type = QUESTIONMARK_LAZY;
				} else
					nodes[j].type = QUESTIONMARK;
				break;

			case '\\': {
				quantifiable = 1;
				if (pattern[++i] == '\0')
					ERROR_THROW(ImpossiblePattern());

				switch (pattern[i]) {
					case 'd':
						nodes[j].type = DIGIT;
						break;
					case 'D':
						nodes[j].type = NOT_DIGIT;
						break;
					case 'w':
						nodes[j].type = ALPHA;
						break;
					case 'W':
						nodes[j].type = NOT_ALPHA;
						break;
					case 's':
						nodes[j].type = WHITESPACE;
						break;
					case 'S':
						nodes[j].type = NOT_WHITESPACE;
						break;
					default:
						nodes[j].type = CHAR;
						nodes[j].u.ch = pattern[i];
						break;
					}
				break;
			}

			case '[': {
				quantifiable = 1;
				if (pattern[i + 1] == '^') {
					i++;
					nodes[j].type = INV_CHAR_CLASS;
				} else
					nodes[j].type = CHAR_CLASS;
				nodes[j].u.cc = buffer + index;

				while (pattern[++i] != ']' && pattern[i] != '\0') {
					int temp = 0;

					if (pattern[i] == '\\') {
						if (pattern[++i] == '\0')
							ERROR_THROW(ImpossiblePattern());
						temp = IS_METACHAR(pattern[i]);
						if (temp || pattern[i] == '\\') {
							if (static_cast<size_t>(index) > bufferLength - 2)
								ERROR_THROW(ExceededBufferLimit());
							buffer[index] = '\\';
						}
					}
					if (static_cast<size_t>(index) > bufferLength - 2)
						ERROR_THROW(ExceededBufferLimit());

					buffer[index++] = pattern[i];

					if (temp)
						continue;
					if (pattern[i] != '-' || pattern[i + 2] == 0 || pattern[i + 2] == ']')
						continue;
					if ((temp = static_cast<unsigned char>(pattern[i + 2] == '\\')) && (pattern[i + 3] == 0 || IS_METACHAR(pattern[i + 3])))
						continue;

					temp = static_cast<unsigned char>(pattern[i + 2 + static_cast<bool>(temp)]);
					if (temp < pattern[i])
						ERROR_THROW(ImpossibleValue());
				}
				if (pattern[i] != ']')
					ERROR_THROW(ImpossiblePattern());
				buffer[index++] = 0;
				break;
			}

			case '{': {
				if (!quantifiable)
					ERROR_THROW(ImpossiblePattern());
				i++;
				quantifiable = 0;
				value = 0;

				do {
					if (pattern[i] == 0 || pattern[i] < '0' || pattern[i] > '9')
						ERROR_THROW(ImpossibleValue());
					value = 10 * value + static_cast<unsigned>(pattern[i++] - '0');
				} while (pattern[i] != ',' && pattern[i] != '}');

				if (value > MAX_QUANTITY)
					ERROR_THROW(ImpossibleValue());

				nodes[j].u.mn[0] = value;

				if (pattern[i] == ',') {
					if (pattern[++i] == 0)
						ERROR_THROW(ImpossiblePattern());
					if (pattern[i] == '}')
						value = MAX_QUANTITY;
					else {
						value = 0;
						while (pattern[i] != '}') {
							if (pattern[i] == 0 || pattern[i] < '0' || pattern[i] > '9')
								ERROR_THROW(ImpossibleValue());
							value = 10 * value + static_cast<unsigned>(pattern[i++] - '0');
						}
						if (value > MAX_QUANTITY || value < nodes[j].u.mn[0])
							ERROR_THROW(ImpossibleValue());
					}
					if (pattern[i + 1] != '\0' && pattern[i + 1] == '?') {
						i++;
						nodes[j].type = QUANTIFIER_LAZY;
					} else
						nodes[j].type = QUANTIFIER;
					nodes[j].u.mn[1] = value;
				} else {
					ERROR_THROW(ImpossiblePattern());
				}
				break;
			}

			default:
				quantifiable = 1;
				nodes[j].type = CHAR;
				nodes[j].u.ch = pattern[i];
				break;
		}
		if (pattern[i] == 0)
			ERROR_THROW(OutOfBoundAccess());
		i++;
		j++;
	}
	nodes[j].type = NONE;
}

Regex::Regex(const std::string &pattern) : _regex() {
	this->compile(pattern);
}