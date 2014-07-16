#ifndef __LDM_ERROR_H__
#define __LDM_ERROR_H__

#include <iostream>

#define LDM_MKERROR(x)	Error(__FILE__, __FUNCTION__, __LINE__, x)

namespace ldm {

class Error
{
private:
	const char* _file;
	const char* _func;
	const char* _desc;
	int _line;
public:
	inline Error(const char* fi, const char* fu, int li, const char* de) {
		_file = fi; _func = fu; _line = li; _desc = de;
	}

	inline std::ostream& Dump(std::ostream& s) const {
		s << "Error: @line " << _line << " in " << _file << "::";
		return s << _func << "(): " << _desc;
	}
};

inline std::ostream& operator << (std::ostream& s, const Error& e) {
	return e.Dump(s);
}

}

#endif
