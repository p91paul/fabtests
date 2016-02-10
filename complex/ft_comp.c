/*
 * Copyright (c) 2013-2015 Intel Corporation.  All rights reserved.
 *
 * This software is available to you under the BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AWV
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdio.h>
#include <string.h>

#include "fabtest.h"


static size_t comp_entry_cnt[] = {
	[FI_CQ_FORMAT_UNSPEC] = 0,
	[FI_CQ_FORMAT_CONTEXT] = FT_COMP_BUF_SIZE / sizeof(struct fi_cq_entry),
	[FI_CQ_FORMAT_MSG] = FT_COMP_BUF_SIZE / sizeof(struct fi_cq_msg_entry),
	[FI_CQ_FORMAT_DATA] = FT_COMP_BUF_SIZE / sizeof(struct fi_cq_data_entry),
	[FI_CQ_FORMAT_TAGGED] = FT_COMP_BUF_SIZE / sizeof(struct fi_cq_tagged_entry)
};

/*
static size_t comp_entry_size[] = {
	[FI_CQ_FORMAT_UNSPEC] = 0,
	[FI_CQ_FORMAT_CONTEXT] = sizeof(struct fi_cq_entry),
	[FI_CQ_FORMAT_MSG] = sizeof(struct fi_cq_msg_entry),
	[FI_CQ_FORMAT_DATA] = sizeof(struct fi_cq_data_entry),
	[FI_CQ_FORMAT_TAGGED] = sizeof(struct fi_cq_tagged_entry)
};
*/


static int ft_open_cqs(void)
{
	struct fi_cq_attr attr;
	int ret;

	if (!txcq) {
		memset(&attr, 0, sizeof attr);
		attr.format = ft_tx_ctrl.cq_format;
		attr.wait_obj = ft_tx_ctrl.comp_wait;
		attr.size = ft_tx_ctrl.max_credits;

		ret = fi_cq_open(domain, &attr, &txcq, NULL);
		if (ret) {
			FT_PRINTERR("fi_cq_open", ret);
			return ret;
		}
	}

	if (!rxcq) {
		memset(&attr, 0, sizeof attr);
		attr.format = ft_rx_ctrl.cq_format;
		attr.wait_obj = ft_rx_ctrl.comp_wait;
		attr.size = ft_rx_ctrl.max_credits;

		ret = fi_cq_open(domain, &attr, &rxcq, NULL);
		if (ret) {
			FT_PRINTERR("fi_cq_open", ret);
			return ret;
		}
	}

	return 0;
}



static int ft_open_cntrs(void)
{
	struct fi_cntr_attr attr;
	int ret;

	if (!txcntr) {
		memset(&attr, 0, sizeof attr);
		attr.wait_obj = ft_tx_ctrl.comp_wait;

		ret = fi_cntr_open(domain, &attr, &txcntr, NULL);
		if (ret) {
			FT_PRINTERR("fi_cntr_open", ret);
			return ret;
		}
	}

	if (!rxcntr) {
		memset(&attr, 0, sizeof attr);
		attr.wait_obj = ft_rx_ctrl.comp_wait;

		ret = fi_cntr_open(domain, &attr, &rxcntr, NULL);
		if (ret) {
			FT_PRINTERR("fi_cntr_open", ret);
			return ret;
		}
	}

	return 0;
}

int ft_open_comp(void)
{
	switch (test_info.comp_type) {
	case FT_COMP_QUEUE:
		return ft_open_cqs();
	case FT_COMP_COUNTER:
		return ft_open_cntrs();
	default:
		return -FI_ENOSYS;
	}
}

int ft_bind_comp(struct fid_ep *ep, uint64_t flags)
{
	int ret;

	if (flags & FI_SEND) {
		if (test_info.comp_type == FT_COMP_QUEUE)
			ret = fi_ep_bind(ep, &txcq->fid, flags & ~FI_RECV);
		else
			ret = fi_ep_bind(ep, &txcntr->fid, flags & ~FI_RECV);
		if (ret) {
			FT_PRINTERR("fi_ep_bind", ret);
			return ret;
		}
	}

	if (flags & FI_RECV) {
		if (test_info.comp_type == FT_COMP_QUEUE)
			ret = fi_ep_bind(ep, &rxcq->fid, flags & ~FI_SEND);
		else
			ret = fi_ep_bind(ep, &rxcntr->fid, flags & ~FI_SEND);
		if (ret) {
			FT_PRINTERR("fi_ep_bind", ret);
			return ret;
		}
	}

	return 0;
}

/* Read CQ until there are no more completions */
#define ft_cq_read(cq_read, cq, buf, count, completions, str, ret, ...)	\
	do {							\
		ret = cq_read(cq, buf, count, ##__VA_ARGS__);	\
		if (ret < 0) {					\
			if (ret == -FI_EAGAIN)			\
				break;				\
			if (ret == -FI_EAVAIL) {		\
				ret = ft_cq_readerr(cq);	\
			} else {				\
				FT_PRINTERR(#cq_read, ret);	\
			}					\
			return ret;				\
		} else {					\
			completions += ret;			\
		}						\
	} while (ret == count)

#define ft_cntr_read(cntr, completions, cur, ret) 	\
	do {											\
		int cntr_val = fi_cntr_read(cntr);			\
		if (cur == cntr_val) {						\
			ret = -FI_EAGAIN;						\
			break;									\
		}											\
		ret = cur - cntr_val;						\
		completions += ret;							\
		cur = cntr_val;								\
	} while (0)									
			

static int ft_comp_x(struct fid_cq *cq, struct fid_cntr* cntr, uint64_t* cur, 
		struct ft_xcontrol *ft_x, const char *x_str, int timeout)
{
	uint8_t buf[FT_COMP_BUF_SIZE];
	struct timespec s, e;
	int poll_time = 0;
	int ret;

	switch(test_info.cq_wait_obj) {
	case FI_WAIT_NONE:
		do {
			if (!poll_time)
				clock_gettime(CLOCK_MONOTONIC, &s);

			if (test_info.comp_type == FT_COMP_QUEUE)
				ft_cq_read(fi_cq_read, cq, buf, comp_entry_cnt[ft_x->cq_format],
						ft_x->credits, x_str, ret);
			else {
				ft_cntr_read(cntr, ft_x->credits, (*cur), ret);
			}

			clock_gettime(CLOCK_MONOTONIC, &e);
			poll_time = get_elapsed(&s, &e, MILLI);
		} while (ret == -FI_EAGAIN && poll_time < timeout);

		break;
	case FI_WAIT_UNSPEC:
	case FI_WAIT_FD:
	case FI_WAIT_MUTEX_COND:
		if (test_info.comp_type == FT_COMP_QUEUE)
			ft_cq_read(fi_cq_sread, cq, buf, comp_entry_cnt[ft_x->cq_format],
				ft_x->credits, x_str, ret, NULL, timeout);
		else {
			ft_cntr_read(cntr, ft_x->credits, (*cur), ret);
			if (ret == -FI_EAGAIN) {
				fi_cntr_wait(cntr, comp_entry_cnt[ft_x->cq_format], timeout);
				ft_cntr_read(cntr, ft_x->credits, (*cur), ret);
			}
		}
		break;
	case FI_WAIT_SET:
		FT_ERR("fi_ubertest: Unsupported cq wait object\n");
		return -1;
	default:
		FT_ERR("Unknown cq wait object\n");
		return -1;
	}

	return (ret == -FI_EAGAIN && timeout) ? ret : 0;
}

int ft_comp_rx(int timeout)
{
	return ft_comp_x(rxcq, rxcntr, &rx_cq_cntr, &ft_rx_ctrl, "rxcq", timeout);
}


int ft_comp_tx(int timeout)
{
	return ft_comp_x(txcq, txcntr, &tx_cq_cntr, &ft_tx_ctrl, "txcq", timeout);
}
