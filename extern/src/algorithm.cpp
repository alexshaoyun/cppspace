// BKDR Hash Function
unsigned int BKDRHash(const char * str)
{
	unsigned int seed = 131; // 31 131 1313 13131 131313 etc..
	unsigned int hash = 0;
	while (*str)
	{
		hash = hash * seed + (*str++);
	}
    
	return (hash & 0x7FFFFFFF);
}
// 使用的时候，一般需要对BKDRHash算法得到的值对hash表长度进行模运算。
