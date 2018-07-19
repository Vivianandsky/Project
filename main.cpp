#include"nginx.h"
#include<iostream>
#include<malloc.h>
#include<assert.h>
using namespace std;

int main()
{
	NgxMemPool pool;
	pool.ngx_create_pool(4096);
	cout << endl;
	cout << pool.ngx_palloc(20) << endl;
	cout << pool.ngx_pnalloc(16) << endl;
	pool.ngx_reset_pool();
	cout << pool.ngx_palloc(15) << endl;
	void * p = pool.ngx_palloc(4097);//´Ólarge·ÖÅä
	cout << p << endl;
	pool.ngx_pfree(p);
	pool.ngx_destroy_pool();
	return 0;
}