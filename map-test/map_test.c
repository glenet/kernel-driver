#include <linux/gfp.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <asm/page.h>
#include <asm/pgtable.h>

#define N_PAGES 8
#define SIZE (PAGE_SIZE / 4)
#define LSIZE (PAGE_SIZE * N_PAGES / 4)

static int compare(void *src, void *dst, int size)
{
	int *isrc = NULL;
	int *idst = NULL;
	int i = 0;
	isrc = (int *)src;
	idst = (int *)dst;

	for (i = 0; i < size; i++) {
		//printk(KERN_INFO "isrc[%d] %d, idst[%d] %d\r\n", i, isrc[i], i, idst[i]);
		if (isrc[i] != idst[i])
			return 0;
	}

	return 1;
}

static void setup_src(void *src, int size)
{
	int i = 0;
	int *tmp = NULL;

	printk(KERN_INFO "setup src: size %d\r\n", size);
	tmp = (int *)src;
	for (i = 0; i < size; i++) {
		if ((i % 2) == 0)
			tmp[i] = 0xff;
		else
			tmp[i] = 0x11;
	}
}

//Test 1: use vmap to get the virtual address
static void testsuit_1(struct page* psrc, struct page* pdst, int isCached)
{
	void *src = NULL, *dst = NULL;
	pgprot_t prot = PAGE_KERNEL;
	if (isCached)
		prot = pgprot_writecombine(prot);
	else
		prot = pgprot_noncached(prot);

	src = vmap(&psrc, 1, 0, prot);
	if (!src) {
		printk(KERN_ERR "src map fail\r\n");
		goto fail_s;
	}
	dst = vmap(&pdst, 1, 0, prot);
	if (!dst) {
		printk(KERN_ERR "dst map fail\r\n");
		goto fail;
	}
	setup_src(src, SIZE);
	memcpy(dst,src, SIZE * 4);
	if (!compare(src, dst, SIZE)) {
		printk(KERN_ERR "Test 1: compare fail\r\n");
		goto fail;
	}
	else
		printk(KERN_INFO "Test 1: compare success\r\n");

fail:
	vunmap(dst);
fail_s:
	vunmap(src);
}

static void testsuit_2(struct page* psrc, struct page* pdst, int isCached)
{
	void *src = NULL, *dst = NULL;
	pgprot_t prot = PAGE_KERNEL;
	if (isCached)
		prot = pgprot_writecombine(prot);
	else
		prot = pgprot_noncached(prot);

	src = vm_map_ram(&psrc, 1, -1, prot);
	if (!src) {
		printk(KERN_ERR "src map fail\r\n");
		goto fail_s;
	}
	dst = vm_map_ram(&pdst, 1, -1, prot);
	if (!dst) {
		printk(KERN_ERR "dst map fail\r\n");
		goto fail;
	}
	memset(src, 0, SIZE * 4);
	memset(dst, 0, SIZE * 4);
	setup_src(src, SIZE);
	memcpy(dst,src, SIZE * 4);
	if (!compare(src, dst, SIZE)) {
		printk(KERN_ERR "Test 2: compare fail\r\n");
		goto fail;
	}
	else
		printk(KERN_INFO "Test 2: compare success\r\n");


fail:
	vm_unmap_ram(dst, 1);
fail_s:
	vm_unmap_ram(src, 1);
}

static void testsuit_3(struct page* psrc[], struct page* pdst[], int isCached)
{
	void *src = NULL, *dst = NULL;
	pgprot_t prot = PAGE_KERNEL;
	if (isCached)
		prot = pgprot_writecombine(prot);
	else
		prot = pgprot_noncached(prot);

	src = vmap(psrc, N_PAGES, 0, prot);
	if (!src) {
		printk(KERN_ERR "src map fail\r\n");
		goto fail_s;
	}
	dst = vmap(pdst, N_PAGES, 0, prot);
	if (!dst) {
		printk(KERN_ERR "dst map fail\r\n");
		goto fail;
	}
	setup_src(src, LSIZE);
	memcpy(dst,src, LSIZE * 4);
	if (!compare(src, dst, LSIZE)) {
		printk(KERN_ERR "Test 3: compare fail\r\n");
		goto fail;
	}
	else
		printk(KERN_INFO "Test 3: compare success\r\n");

fail:
	vunmap(dst);
fail_s:
	vunmap(src);
}

static void testsuit_4(struct page* psrc[], struct page* pdst[], int isCached)
{
	void *src = NULL, *dst = NULL;
	pgprot_t prot = PAGE_KERNEL;
	if (isCached)
		prot = pgprot_writecombine(prot);
	else
		prot = pgprot_noncached(prot);

	src = vm_map_ram(psrc, N_PAGES, -1, prot);
	if (!src) {
		printk(KERN_ERR "src map fail\r\n");
		goto fail_s;
	}
	dst = vm_map_ram(pdst, N_PAGES, -1, prot);
	if (!dst) {
		printk(KERN_ERR "dst map fail\r\n");
		goto fail;
	}
	memset(src, 0, LSIZE * 4);
	memset(dst, 0, LSIZE * 4);
	setup_src(src, LSIZE);
	memcpy(dst,src, LSIZE * 4);
	if (!compare(src, dst, LSIZE)) {
		printk(KERN_ERR "Test 4: compare fail\r\n");
		goto fail;
	}
	else
		printk(KERN_INFO "Test 4: compare success\r\n");

fail:
	vm_unmap_ram(dst, N_PAGES);
fail_s:
	vm_unmap_ram(src, N_PAGES);
}

static int __init map_test_init(void)
{
	struct page* pf = NULL;
	struct page* ps = NULL;
	struct page** pfl;
	struct page** psl;
	struct page* p1 = NULL;
	struct page* p2 = NULL;
	int i = 0;

	printk(KERN_INFO "map test module init\r\n");

	pf = alloc_page(__GFP_HIGHMEM | __GFP_ZERO);
	if (!pf) {
		printk(KERN_ERR "pf: alloc page fail\r\n");
		goto fail;
	}
	ps = alloc_page(__GFP_HIGHMEM | __GFP_ZERO);
	if (!ps) {
		printk(KERN_ERR "ps: alloc page fail\r\n");
		goto fail_pf;
	}

	pfl = kzalloc(sizeof(struct page *) * N_PAGES, GFP_KERNEL);
	if (!pfl) {
		printk(KERN_ERR "pfl: alloc  fail\r\n");
		goto fail_ps;
	}
	psl = kzalloc(sizeof(struct page *) * N_PAGES, GFP_KERNEL);
	if (!psl) {
		printk(KERN_ERR "psl: alloc  fail\r\n");
		goto fail_free_kpfl;
	}
	for (i = 0; i < N_PAGES; i++) {
		p1 = alloc_page(__GFP_HIGHMEM | __GFP_ZERO);
		if (!p1) {
			printk(KERN_ERR "p1: alloc page fail\r\n");
			goto fail_free_pfl;
		}
		p2 = alloc_page(__GFP_HIGHMEM | __GFP_ZERO);
		if (!p2) {
			printk(KERN_ERR "p2: alloc page fail\r\n");
			goto fail_free_psl;
		}
		pfl[i] = p1;
		psl[i] = p2;
	}

	testsuit_1(pf, ps, 1);
	testsuit_2(pf, ps, 1);
	testsuit_3(pfl, psl, 1);
	testsuit_4(pfl, psl, 1);
	testsuit_1(pf, ps, 0);
	testsuit_2(pf, ps, 0);
	testsuit_3(pfl, psl, 0);
	testsuit_4(pfl, psl, 0);

fail_free_psl:
	for (i = 0; i < N_PAGES; i++) {
		__free_page(psl[i]);
	}
fail_free_pfl:
	for (i = 0; i < N_PAGES; i++) {
		__free_page(pfl[i]);
	}
	kfree(psl);
fail_free_kpfl:
	kfree(pfl);
fail_ps:
	__free_page(ps);
fail_pf:
	__free_page(pf);
fail:
	return 0;
}

static void __exit map_test_exit(void)
{
	printk(KERN_INFO "map test module exit\r\n");
}

module_init(map_test_init);
module_exit(map_test_exit);

MODULE_DESCRIPTION("vm_map_ram testing");
MODULE_AUTHOR("Michael Yang");
MODULE_LICENSE("GPL");
