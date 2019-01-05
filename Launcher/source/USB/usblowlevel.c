
static signed int hId = -1;
static void run()
{
	if (hId == -1)
		hId = iosCreateHeap(8192);
	if (hId < 0)
		return;
	memset(&hId, 0, 8192);
	memset((void*)0x901C4900, 0, 37);


}