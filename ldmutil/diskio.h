#ifndef __LDM_DISKIO_H__
#define __LDM_DISKIO_H__

#include <climits>
#include "types.h"

namespace ldm {

class diskio {
private:
	u64	_size;
	bool	_open;
	bool	_readonly;
	int	_fd;
	u64	_pos;
public:
	diskio(void);
	~diskio(void);
	void Open(const char* filename, bool readonly = true);
	void Close(void);
	static int GetSectorSize(void);
	void SetPos(int sector);
	int  GetPos(void);
	void Write(const void* src, int nsect = 1, int pos = INT_MIN);
	void Read(void* dest, int nsect = 1, int pos = INT_MIN);
	int  GetSize(void);
};

}
#endif
