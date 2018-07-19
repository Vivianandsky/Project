#include"nginx.h"
#include<iostream>
#include<malloc.h>
#include<assert.h>
using namespace std;

void NgxMemPool::ngx_create_pool(size_t size)
{
	_pool = new ngx_pool_t[size / sizeof(ngx_pool_t)];

	assert(_pool != NULL);

	_pool->d.last = (u_char *)_pool + sizeof(ngx_pool_t);
	_pool->d.end = (u_char *)_pool + size;
	_pool->d.next = NULL;
	_pool->d.failed = 0;

	size = size - sizeof(ngx_pool_t);
	_pool->max = (size < NGX_MAX_ALLOC_FROM_POOL) ? size : NGX_MAX_ALLOC_FROM_POOL;

	_pool->current = _pool;
	_pool->large = NULL;
}

void NgxMemPool::ngx_destroy_pool()
{
	ngx_pool_large_t    *l;
	ngx_pool_t *p=_pool;
	ngx_pool_t *n = p->d.next;
	for (l = _pool->large; l!=NULL; l = l->next) 
	{
		if (l->alloc!=NULL) 
		{
			free(l->alloc);
		}
	}

	for (p = _pool, n = _pool->d.next; ; p = n, n = n->d.next)
	{
		free(p);

		if (n == NULL)
		{
			break;
		}
	}
}

void NgxMemPool::ngx_reset_pool()
{
	ngx_pool_t        *p;
	ngx_pool_large_t  *l=_pool->large;

	for (l = _pool->large; l; l = l->next) 
	{
		if (l->alloc) 
		{
			delete(l->alloc);
		}
	}

	for (p = _pool; p; p = p->d.next) 
	{
		p->d.last = (u_char *)p + sizeof(ngx_pool_t);
		p->d.failed = 0;
	}

	_pool->current = _pool;
	_pool->large = NULL;
}
void *NgxMemPool::ngx_palloc_small(ngx_pool_t *_pool, size_t size, int a)
{
	u_char      *m;
	ngx_pool_t  *p=_pool->current;

	do {
		m = p->d.last;
		if ((size_t)(p->d.end - m) >= size) 
		{
			p->d.last = m + size;
			return m;
		}
		p = p->d.next;
	} while (p);
	return ngx_palloc_block(_pool, size);
}
void *NgxMemPool::ngx_palloc_large(ngx_pool_t *_pool, size_t size)
{
	   void *p=malloc(size);
    ngx_uint_t   n=0;
	ngx_pool_large_t  *large = _pool->large;

	p = new ngx_pool_t[size / sizeof(ngx_pool_t)];
    if (p == NULL) {
        return NULL;
    }

    n = 0;

    while(large!=NULL)
	{
        if (large->alloc == NULL) 
		{
            large->alloc = p;
            return p;
        }

        if (n++ > 3) 
		{
            break;
        }
    }

    large =(ngx_pool_large_t*) ngx_palloc_small(_pool,sizeof(ngx_pool_large_t),0);
    if (large == NULL) 
	{
		delete(p);
        return NULL;
    }

    large->alloc = p;
    large->next = _pool->large;
    _pool->large = large;

    return p;
}
void* NgxMemPool::ngx_palloc_block(ngx_pool_t *_pool, size_t size)
{
	u_char *m = (u_char*)malloc(size);
	size_t psize = (size_t)(_pool->d.end - (u_char*)_pool);

	psize = (size_t)(_pool->d.end - (u_char*)_pool);//计算内存池第一个内存块大小
	ngx_pool_t *newpool = (ngx_pool_t*)m;

	newpool->d.end = m + psize;
	newpool->d.next = NULL;
	newpool->d.failed = 0;

	m += sizeof(ngx_pool_data_t);
	m = ngx_align_ptr(m, sizeof(unsigned long));
	newpool->d.last = m + size;

	ngx_pool_t *p = _pool->current;
	while (p->d.next != NULL)
	{
		if (p->d.failed++ > 4)
		{
			_pool->current = p->d.next;
		}
		p = p->d.next;
	}
	p->d.next = newpool;

	return m;
}
void* NgxMemPool::ngx_palloc(size_t size)
{
	if (size <= _pool->max)
	{
		return ngx_palloc_small(_pool, size, 1);
	}
	//大块内存分配
	return ngx_palloc_large(_pool, size);
}
void* NgxMemPool::ngx_pnalloc(size_t size)
{
	if (size <= _pool->max)
	{
		return ngx_palloc_small(_pool, size, 0);
	}
	return ngx_palloc_large(_pool, size);
}

void NgxMemPool::ngx_pfree(void *p)
{
	ngx_pool_large_t  *l;
	for (l = _pool->large; l; l = l->next) 
	{
		if (_pool == l->alloc) 
		{
			free(l->alloc);
			l->alloc = NULL;
		}
	}
}