#include "../git-compat-util.h"

ssize_t git_pread(int fd, void *buf, size_t count, off_t offset)
{
        off_t current_offset;
        char *p = buf;
        ssize_t total = 0;

        current_offset = lseek(fd, 0, SEEK_CUR);

        if (lseek(fd, offset, SEEK_SET) < 0)
                return -1;

        while (count > 0) {
                ssize_t loaded = xread(fd, p, count);
                if (loaded <= 0) {
                        total = (total ? total : loaded);
                        break;
                }
                count -= loaded;
                p += loaded;
                total += loaded;
        }

        if (current_offset != lseek(fd, current_offset, SEEK_SET))
                return -1;
        return total;
}
