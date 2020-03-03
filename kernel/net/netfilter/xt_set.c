// SPDX-License-Identifier: GPL-2.0-only
/* Copyright (C) 2000-2002 Joakim Axelsson <gozem@linux.nu>
 *                         Patrick Schaaf <bof@bof.de>
 *                         Martin Josefsson <gandalf@wlug.westbo.se>
 * Copyright (C) 2003-2013 Jozsef Kadlecsik <kadlec@netfilter.org>
 */

/* Kernel module which implements the set match and SET target
 * for netfilter/iptables.
 */

#include <linux/module.h>
#include <linux/skbuff.h>

#include <linux/netfilter/x_tables.h>
#include <linux/netfilter/ipset/ip_set.h>
#include <uapi/linux/netfilter/xt_set.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jozsef Kadlecsik <kadlec@netfilter.org>");
MODULE_DESCRIPTION("Xtables: IP set match and target module");
MODULE_ALIAS("xt_SET");
MODULE_ALIAS("ipt_set");
MODULE_ALIAS("ip6t_set");
MODULE_ALIAS("ipt_SET");
MODULE_ALIAS("ip6t_SET");

#ifdef HAVE_CHECKENTRY_BOOL
#define CHECK_OK	1
#define CHECK_FAIL(err)	0
#define	CONST		const
#define FTYPE		bool
#else /* LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,35) */
#define CHECK_OK	0
#define CHECK_FAIL(err)	(err)
#define	CONST
#define	FTYPE		int
#endif
#ifdef HAVE_XT_MTCHK_PARAM_STRUCT_NET
#define XT_PAR_NET(par)	((par)->net)
#else
#define	XT_PAR_NET(par)	(&(init_net))
#endif

static inline int
match_set(ip_set_id_t index, const struct sk_buff *skb,
	  const struct xt_action_param *par,
	  struct ip_set_adt_opt *opt, int inv)
{
	if (ip_set_test(index, skb, par, opt))
		inv = !inv;
	return inv;
}

#define ADT_OPT(n, f, d, fs, pd, cfs, t, p, b, po, bo)	\
struct ip_set_adt_opt n = {				\
	.family	= f,					\
	.dim = d,					\
	.flags = fs,					\
	.physdev = pd,					\
	.cmdflags = cfs,				\
	.ext.timeout = t,				\
	.ext.packets = p,				\
	.ext.bytes = b,					\
	.ext.packets_op = po,				\
	.ext.bytes_op = bo,				\
}

/* Revision 0 interface: backward compatible with netfilter/iptables */

static bool
set_match_v0(const struct sk_buff *skb, CONST struct xt_action_param *par)
{
	const struct xt_set_info_match_v0 *info = par->matchinfo;

	ADT_OPT(opt, XT_FAMILY(par), info->match_set.u.compat.dim,
		info->match_set.u.compat.flags, 0, 0, UINT_MAX,
		0, 0, 0, 0);

	return match_set(info->match_set.index, skb, par, &opt,
			 info->match_set.u.compat.flags & IPSET_INV_MATCH);
}

static void
compat_flags(struct xt_set_info_v0 *info)
{
	u_int8_t i;

	/* Fill out compatibility data according to enum ip_set_kopt */
	info->u.compat.dim = IPSET_DIM_ZERO;
	if (info->u.flags[0] & IPSET_MATCH_INV)
		info->u.compat.flags |= IPSET_INV_MATCH;
	for (i = 0; i < IPSET_DIM_MAX - 1 && info->u.flags[i]; i++) {
		info->u.compat.dim++;
		if (info->u.flags[i] & IPSET_SRC)
			info->u.compat.flags |= (1 << info->u.compat.dim);
	}
}

static FTYPE
set_match_v0_checkentry(const struct xt_mtchk_param *par)
{
	struct xt_set_info_match_v0 *info = par->matchinfo;
	ip_set_id_t index;

	index = ip_set_nfnl_get_byindex(XT_PAR_NET(par), info->match_set.index);

	if (index == IPSET_INVALID_ID) {
		pr_warn("Cannot find set identified by id %u to match\n",
			info->match_set.index);
		return CHECK_FAIL(-ENOENT);
	}
	if (info->match_set.u.flags[IPSET_DIM_MAX - 1] != 0) {
		pr_warn("Protocol error: set match dimension is over the limit!\n");
		ip_set_nfnl_put(XT_PAR_NET(par), info->match_set.index);
		return CHECK_FAIL(-ERANGE);
	}

	/* Fill out compatibility data */
	compat_flags(&info->match_set);

	return CHECK_OK;
}

static void
set_match_v0_destroy(const struct xt_mtdtor_param *par)
{
	struct xt_set_info_match_v0 *info = par->matchinfo;

	ip_set_nfnl_put(XT_PAR_NET(par), info->match_set.index);
}

/* Revision 1 */

static bool
set_match_v1(const struct sk_buff *skb, CONST struct xt_action_param *par)
{
	const struct xt_set_info_match_v1 *info = par->matchinfo;

	ADT_OPT(opt, XT_FAMILY(par), info->match_set.dim,
		info->match_set.flags, 0, 0, UINT_MAX,
		0, 0, 0, 0);

	if (opt.flags & IPSET_RETURN_NOMATCH)
		opt.cmdflags |= IPSET_FLAG_RETURN_NOMATCH;

	return match_set(info->match_set.index, skb, par, &opt,
			 info->match_set.flags & IPSET_INV_MATCH);
}

static FTYPE
set_match_v1_checkentry(const struct xt_mtchk_param *par)
{
	struct xt_set_info_match_v1 *info = par->matchinfo;
	ip_set_id_t index;

	index = ip_set_nfnl_get_byindex(XT_PAR_NET(par), info->match_set.index);

	if (index == IPSET_INVALID_ID) {
		pr_warn("Cannot find set identified by id %u to match\n",
			info->match_set.index);
		return CHECK_FAIL(-ENOENT);
	}
	if (info->match_set.dim > IPSET_DIM_MAX) {
		pr_warn("Protocol error: set match dimension is over the limit!\n");
		ip_set_nfnl_put(XT_PAR_NET(par), info->match_set.index);
		return CHECK_FAIL(-ERANGE);
	}

	return CHECK_OK;
}

static void
set_match_v1_destroy(const struct xt_mtdtor_param *par)
{
	struct xt_set_info_match_v1 *info = par->matchinfo;

	ip_set_nfnl_put(XT_PAR_NET(par), info->match_set.index);
}

/* Revision 3 match */

static bool
set_match_v3(const struct sk_buff *skb, CONST struct xt_action_param *par)
{
	const struct xt_set_info_match_v3 *info = par->matchinfo;

	ADT_OPT(opt, XT_FAMILY(par), info->match_set.dim,
		info->match_set.flags, 0, info->flags, UINT_MAX,
		info->packets.value, info->bytes.value,
		info->packets.op, info->bytes.op);

	if (info->flags & IPSET_FLAG_PHYSDEV)
		opt.physdev |= IPSET_DIM_MASK;

	if (info->packets.op != IPSET_COUNTER_NONE ||
	    info->bytes.op != IPSET_COUNTER_NONE)
		opt.cmdflags |= IPSET_FLAG_MATCH_COUNTERS;

	return match_set(info->match_set.index, skb, par, &opt,
			 info->match_set.flags & IPSET_INV_MATCH);
}

#define set_match_v3_checkentry	set_match_v1_checkentry
#define set_match_v3_destroy	set_match_v1_destroy

/* Revision 4 match */

static bool
set_match_v4(const struct sk_buff *skb, CONST struct xt_action_param *par)
{
	const struct xt_set_info_match_v4 *info = par->matchinfo;

	ADT_OPT(opt, XT_FAMILY(par), info->match_set.dim,
		info->match_set.flags, 0, info->flags, UINT_MAX,
		info->packets.value, info->bytes.value,
		info->packets.op, info->bytes.op);

	if (par->match->revision > 4)
		opt.physdev =
			((const struct xt_set_info_match_v5 *)info)->physdev;
	else if (info->flags & IPSET_FLAG_PHYSDEV)
		opt.physdev |= IPSET_DIM_MASK;

	if (info->packets.op != IPSET_COUNTER_NONE ||
	    info->bytes.op != IPSET_COUNTER_NONE)
		opt.cmdflags |= IPSET_FLAG_MATCH_COUNTERS;

	return match_set(info->match_set.index, skb, par, &opt,
			 info->match_set.flags & IPSET_INV_MATCH);
}

#define set_match_v4_checkentry	set_match_v1_checkentry
#define set_match_v4_destroy	set_match_v1_destroy

/* Revision 5 match */

#define set_match_v5		set_match_v4
#define set_match_v5_checkentry	set_match_v1_checkentry
#define set_match_v5_destroy	set_match_v1_destroy

/* Revision 0 interface: backward compatible with netfilter/iptables */

#ifdef HAVE_XT_TARGET_PARAM
#undef xt_action_param
#define xt_action_param	xt_target_param
#define CAST_TO_MATCH	(const struct xt_match_param *)
#else
#define	CAST_TO_MATCH
#endif

static unsigned int
set_target_v0(struct sk_buff *skb, const struct xt_action_param *par)
{
	const struct xt_set_info_target_v0 *info = par->targinfo;

	ADT_OPT(add_opt, XT_FAMILY(par), info->add_set.u.compat.dim,
		info->add_set.u.compat.flags, 0, 0, UINT_MAX,
		0, 0, 0, 0);
	ADT_OPT(del_opt, XT_FAMILY(par), info->del_set.u.compat.dim,
		info->del_set.u.compat.flags, 0, 0, UINT_MAX,
		0, 0, 0, 0);

	if (info->add_set.index != IPSET_INVALID_ID)
		ip_set_add(info->add_set.index, skb, CAST_TO_MATCH par,
			   &add_opt);
	if (info->del_set.index != IPSET_INVALID_ID)
		ip_set_del(info->del_set.index, skb, CAST_TO_MATCH par,
			   &del_opt);

	return XT_CONTINUE;
}

static FTYPE
set_target_v0_checkentry(const struct xt_tgchk_param *par)
{
	struct xt_set_info_target_v0 *info = par->targinfo;
	ip_set_id_t index;

	if (info->add_set.index != IPSET_INVALID_ID) {
		index = ip_set_nfnl_get_byindex(XT_PAR_NET(par),
						info->add_set.index);
		if (index == IPSET_INVALID_ID) {
			pr_warn("Cannot find add_set index %u as target\n",
				info->add_set.index);
			return CHECK_FAIL(-ENOENT);
		}
	}

	if (info->del_set.index != IPSET_INVALID_ID) {
		index = ip_set_nfnl_get_byindex(XT_PAR_NET(par),
						info->del_set.index);
		if (index == IPSET_INVALID_ID) {
			pr_warn("Cannot find del_set index %u as target\n",
				info->del_set.index);
			if (info->add_set.index != IPSET_INVALID_ID)
				ip_set_nfnl_put(XT_PAR_NET(par),
						info->add_set.index);
			return CHECK_FAIL(-ENOENT);
		}
	}
	if (info->add_set.u.flags[IPSET_DIM_MAX - 1] != 0 ||
	    info->del_set.u.flags[IPSET_DIM_MAX - 1] != 0) {
		pr_warn("Protocol error: SET target dimension is over the limit!\n");
		if (info->add_set.index != IPSET_INVALID_ID)
			ip_set_nfnl_put(XT_PAR_NET(par), info->add_set.index);
		if (info->del_set.index != IPSET_INVALID_ID)
			ip_set_nfnl_put(XT_PAR_NET(par), info->del_set.index);
		return CHECK_FAIL(-ERANGE);
	}

	/* Fill out compatibility data */
	compat_flags(&info->add_set);
	compat_flags(&info->del_set);

	return CHECK_OK;
}

static void
set_target_v0_destroy(const struct xt_tgdtor_param *par)
{
	const struct xt_set_info_target_v0 *info = par->targinfo;

	if (info->add_set.index != IPSET_INVALID_ID)
		ip_set_nfnl_put(XT_PAR_NET(par), info->add_set.index);
	if (info->del_set.index != IPSET_INVALID_ID)
		ip_set_nfnl_put(XT_PAR_NET(par), info->del_set.index);
}

/* Revision 1 target */

static unsigned int
set_target_v1(struct sk_buff *skb, const struct xt_action_param *par)
{
	const struct xt_set_info_target_v1 *info = par->targinfo;

	ADT_OPT(add_opt, XT_FAMILY(par), info->add_set.dim,
		info->add_set.flags, 0, 0, UINT_MAX,
		0, 0, 0, 0);
	ADT_OPT(del_opt, XT_FAMILY(par), info->del_set.dim,
		info->del_set.flags, 0, 0, UINT_MAX,
		0, 0, 0, 0);

	if (info->add_set.index != IPSET_INVALID_ID)
		ip_set_add(info->add_set.index, skb, CAST_TO_MATCH par,
			   &add_opt);
	if (info->del_set.index != IPSET_INVALID_ID)
		ip_set_del(info->del_set.index, skb, CAST_TO_MATCH par,
			   &del_opt);

	return XT_CONTINUE;
}

static FTYPE
set_target_v1_checkentry(const struct xt_tgchk_param *par)
{
	const struct xt_set_info_target_v1 *info = par->targinfo;
	ip_set_id_t index;

	if (info->add_set.index != IPSET_INVALID_ID) {
		index = ip_set_nfnl_get_byindex(XT_PAR_NET(par),
						info->add_set.index);
		if (index == IPSET_INVALID_ID) {
			pr_warn("Cannot find add_set index %u as target\n",
				info->add_set.index);
			return CHECK_FAIL(-ENOENT);
		}
	}

	if (info->del_set.index != IPSET_INVALID_ID) {
		index = ip_set_nfnl_get_byindex(XT_PAR_NET(par),
						info->del_set.index);
		if (index == IPSET_INVALID_ID) {
			pr_warn("Cannot find del_set index %u as target\n",
				info->del_set.index);
			if (info->add_set.index != IPSET_INVALID_ID)
				ip_set_nfnl_put(XT_PAR_NET(par),
						info->add_set.index);
			return CHECK_FAIL(-ENOENT);
		}
	}
	if (info->add_set.dim > IPSET_DIM_MAX ||
	    info->del_set.dim > IPSET_DIM_MAX) {
		pr_warn("Protocol error: SET target dimension is over the limit!\n");
		if (info->add_set.index != IPSET_INVALID_ID)
			ip_set_nfnl_put(XT_PAR_NET(par), info->add_set.index);
		if (info->del_set.index != IPSET_INVALID_ID)
			ip_set_nfnl_put(XT_PAR_NET(par), info->del_set.index);
		return CHECK_FAIL(-ERANGE);
	}

	return CHECK_OK;
}

static void
set_target_v1_destroy(const struct xt_tgdtor_param *par)
{
	const struct xt_set_info_target_v1 *info = par->targinfo;

	if (info->add_set.index != IPSET_INVALID_ID)
		ip_set_nfnl_put(XT_PAR_NET(par), info->add_set.index);
	if (info->del_set.index != IPSET_INVALID_ID)
		ip_set_nfnl_put(XT_PAR_NET(par), info->del_set.index);
}

/* Revision 2 target */

static unsigned int
set_target_v2(struct sk_buff *skb, const struct xt_action_param *par)
{
	const struct xt_set_info_target_v2 *info = par->targinfo;

	ADT_OPT(add_opt, XT_FAMILY(par), info->add_set.dim,
		info->add_set.flags, 0, info->flags, info->timeout,
		0, 0, 0, 0);
	ADT_OPT(del_opt, XT_FAMILY(par), info->del_set.dim,
		info->del_set.flags, 0, 0, UINT_MAX,
		0, 0, 0, 0);

	if (info->flags & IPSET_FLAG_PHYSDEV)
		add_opt.physdev |= IPSET_DIM_MASK;

	/* Normalize to fit into jiffies */
	if (add_opt.ext.timeout != IPSET_NO_TIMEOUT &&
	    add_opt.ext.timeout > IPSET_MAX_TIMEOUT)
		add_opt.ext.timeout = IPSET_MAX_TIMEOUT;
	if (info->add_set.index != IPSET_INVALID_ID)
		ip_set_add(info->add_set.index, skb, CAST_TO_MATCH par,
			   &add_opt);
	if (info->del_set.index != IPSET_INVALID_ID)
		ip_set_del(info->del_set.index, skb, CAST_TO_MATCH par,
			   &del_opt);

	return XT_CONTINUE;
}

#define set_target_v2_checkentry	set_target_v1_checkentry
#define set_target_v2_destroy		set_target_v1_destroy

/* Revision 3 target */

#define MOPT(opt, member)	((opt).ext.skbinfo.member)

static unsigned int
set_target_v3(struct sk_buff *skb, const struct xt_action_param *par)
{
	const struct xt_set_info_target_v3 *info = par->targinfo;
	int ret;

	ADT_OPT(add_opt, XT_FAMILY(par), info->add_set.dim,
		info->add_set.flags, 0, info->flags, info->timeout,
		0, 0, 0, 0);
	ADT_OPT(del_opt, XT_FAMILY(par), info->del_set.dim,
		info->del_set.flags, 0, 0, UINT_MAX,
		0, 0, 0, 0);
	ADT_OPT(map_opt, XT_FAMILY(par), info->map_set.dim,
		info->map_set.flags, 0, 0, UINT_MAX,
		0, 0, 0, 0);

	if (par->target->revision > 3)
		add_opt.physdev =
			((const struct xt_set_info_target_v4 *)info)->physdev;
	else if (info->flags & IPSET_FLAG_PHYSDEV)
		add_opt.physdev |= IPSET_DIM_MASK;

	/* Normalize to fit into jiffies */
	if (add_opt.ext.timeout != IPSET_NO_TIMEOUT &&
	    add_opt.ext.timeout > IPSET_MAX_TIMEOUT)
		add_opt.ext.timeout = IPSET_MAX_TIMEOUT;
	if (info->add_set.index != IPSET_INVALID_ID)
		ip_set_add(info->add_set.index, skb, CAST_TO_MATCH par,
			   &add_opt);
	if (info->del_set.index != IPSET_INVALID_ID)
		ip_set_del(info->del_set.index, skb, CAST_TO_MATCH par,
			   &del_opt);
	if (info->map_set.index != IPSET_INVALID_ID) {
		map_opt.cmdflags |= info->flags & (IPSET_FLAG_MAP_SKBMARK |
						   IPSET_FLAG_MAP_SKBPRIO |
						   IPSET_FLAG_MAP_SKBQUEUE);
		ret = match_set(info->map_set.index, skb, CAST_TO_MATCH par,
				&map_opt,
				info->map_set.flags & IPSET_INV_MATCH);
		if (!ret)
			return XT_CONTINUE;
		if (map_opt.cmdflags & IPSET_FLAG_MAP_SKBMARK)
			skb->mark = (skb->mark & ~MOPT(map_opt,skbmarkmask))
				    ^ MOPT(map_opt, skbmark);
		if (map_opt.cmdflags & IPSET_FLAG_MAP_SKBPRIO)
			skb->priority = MOPT(map_opt, skbprio);
		if ((map_opt.cmdflags & IPSET_FLAG_MAP_SKBQUEUE) &&
		    skb->dev &&
		    skb->dev->real_num_tx_queues > MOPT(map_opt, skbqueue))
			skb_set_queue_mapping(skb, MOPT(map_opt, skbqueue));
	}
	return XT_CONTINUE;
}

static FTYPE
set_target_v3_checkentry(const struct xt_tgchk_param *par)
{
	const struct xt_set_info_target_v3 *info = par->targinfo;
	ip_set_id_t index;
	int ret = 0;

	if (info->add_set.index != IPSET_INVALID_ID) {
		index = ip_set_nfnl_get_byindex(XT_PAR_NET(par),
						info->add_set.index);
		if (index == IPSET_INVALID_ID) {
			pr_warn("Cannot find add_set index %u as target\n",
				info->add_set.index);
			return CHECK_FAIL(-ENOENT);
		}
	}

	if (info->del_set.index != IPSET_INVALID_ID) {
		index = ip_set_nfnl_get_byindex(XT_PAR_NET(par),
						info->del_set.index);
		if (index == IPSET_INVALID_ID) {
			pr_warn("Cannot find del_set index %u as target\n",
				info->del_set.index);
			ret = -ENOENT;
			goto cleanup_add;
		}
	}

	if (info->map_set.index != IPSET_INVALID_ID) {
		if (((info->flags & IPSET_FLAG_MAP_SKBPRIO) |
		     (info->flags & IPSET_FLAG_MAP_SKBQUEUE)) &&
		     (par->hook_mask & ~(1 << NF_INET_FORWARD |
					 1 << NF_INET_LOCAL_OUT |
					 1 << NF_INET_POST_ROUTING))) {
			pr_warn("mapping of prio or/and queue is allowed only from OUTPUT/FORWARD/POSTROUTING chains\n");
			ret = -EINVAL;
			goto cleanup_del;
		}
		index = ip_set_nfnl_get_byindex(XT_PAR_NET(par),
						info->map_set.index);
		if (index == IPSET_INVALID_ID) {
			pr_warn("Cannot find map_set index %u as target\n",
				info->map_set.index);
			ret = -ENOENT;
			goto cleanup_del;
		}
	}

	if (info->add_set.dim > IPSET_DIM_MAX ||
	    info->del_set.dim > IPSET_DIM_MAX ||
	    info->map_set.dim > IPSET_DIM_MAX) {
		pr_warn("Protocol error: SET target dimension is over the limit!\n");
		ret = -ERANGE;
		goto cleanup_mark;
	}

	return CHECK_OK;
cleanup_mark:
	if (info->map_set.index != IPSET_INVALID_ID)
		ip_set_nfnl_put(XT_PAR_NET(par), info->map_set.index);
cleanup_del:
	if (info->del_set.index != IPSET_INVALID_ID)
		ip_set_nfnl_put(XT_PAR_NET(par), info->del_set.index);
cleanup_add:
	if (info->add_set.index != IPSET_INVALID_ID)
		ip_set_nfnl_put(XT_PAR_NET(par), info->add_set.index);
	return CHECK_FAIL(ret);
}

static void
set_target_v3_destroy(const struct xt_tgdtor_param *par)
{
	const struct xt_set_info_target_v3 *info = par->targinfo;

	if (info->add_set.index != IPSET_INVALID_ID)
		ip_set_nfnl_put(XT_PAR_NET(par), info->add_set.index);
	if (info->del_set.index != IPSET_INVALID_ID)
		ip_set_nfnl_put(XT_PAR_NET(par), info->del_set.index);
	if (info->map_set.index != IPSET_INVALID_ID)
		ip_set_nfnl_put(XT_PAR_NET(par), info->map_set.index);
}

/* Revision 4 target */

#define set_target_v4			set_target_v3
#define set_target_v4_checkentry	set_target_v3_checkentry
#define set_target_v4_destroy		set_target_v3_destroy

static struct xt_match set_matches[] __read_mostly = {
	{
		.name		= "set",
		.family		= NFPROTO_IPV4,
		.revision	= 0,
		.match		= set_match_v0,
		.matchsize	= sizeof(struct xt_set_info_match_v0),
		.checkentry	= set_match_v0_checkentry,
		.destroy	= set_match_v0_destroy,
		.me		= THIS_MODULE
	},
	{
		.name		= "set",
		.family		= NFPROTO_IPV4,
		.revision	= 1,
		.match		= set_match_v1,
		.matchsize	= sizeof(struct xt_set_info_match_v1),
		.checkentry	= set_match_v1_checkentry,
		.destroy	= set_match_v1_destroy,
		.me		= THIS_MODULE
	},
	{
		.name		= "set",
		.family		= NFPROTO_IPV6,
		.revision	= 1,
		.match		= set_match_v1,
		.matchsize	= sizeof(struct xt_set_info_match_v1),
		.checkentry	= set_match_v1_checkentry,
		.destroy	= set_match_v1_destroy,
		.me		= THIS_MODULE
	},
	/* --return-nomatch flag support */
	{
		.name		= "set",
		.family		= NFPROTO_IPV4,
		.revision	= 2,
		.match		= set_match_v1,
		.matchsize	= sizeof(struct xt_set_info_match_v1),
		.checkentry	= set_match_v1_checkentry,
		.destroy	= set_match_v1_destroy,
		.me		= THIS_MODULE
	},
	{
		.name		= "set",
		.family		= NFPROTO_IPV6,
		.revision	= 2,
		.match		= set_match_v1,
		.matchsize	= sizeof(struct xt_set_info_match_v1),
		.checkentry	= set_match_v1_checkentry,
		.destroy	= set_match_v1_destroy,
		.me		= THIS_MODULE
	},
	/* counters support: update, match */
	{
		.name		= "set",
		.family		= NFPROTO_IPV4,
		.revision	= 3,
		.match		= set_match_v3,
		.matchsize	= sizeof(struct xt_set_info_match_v3),
		.checkentry	= set_match_v3_checkentry,
		.destroy	= set_match_v3_destroy,
		.me		= THIS_MODULE
	},
	{
		.name		= "set",
		.family		= NFPROTO_IPV6,
		.revision	= 3,
		.match		= set_match_v3,
		.matchsize	= sizeof(struct xt_set_info_match_v3),
		.checkentry	= set_match_v3_checkentry,
		.destroy	= set_match_v3_destroy,
		.me		= THIS_MODULE
	},
	/* new revision for counters support: update, match */
	{
		.name		= "set",
		.family		= NFPROTO_IPV4,
		.revision	= 4,
		.match		= set_match_v4,
		.matchsize	= sizeof(struct xt_set_info_match_v4),
		.checkentry	= set_match_v4_checkentry,
		.destroy	= set_match_v4_destroy,
		.me		= THIS_MODULE
	},
	{
		.name		= "set",
		.family		= NFPROTO_IPV6,
		.revision	= 4,
		.match		= set_match_v4,
		.matchsize	= sizeof(struct xt_set_info_match_v4),
		.checkentry	= set_match_v4_checkentry,
		.destroy	= set_match_v4_destroy,
		.me		= THIS_MODULE
	},
	/* proper support for "physdev:" prefix */
	{
		.name		= "set",
		.family		= NFPROTO_IPV4,
		.revision	= 5,
		.match		= set_match_v5,
		.matchsize	= sizeof(struct xt_set_info_match_v5),
		.checkentry	= set_match_v5_checkentry,
		.destroy	= set_match_v5_destroy,
		.me		= THIS_MODULE
	},
	{
		.name		= "set",
		.family		= NFPROTO_IPV6,
		.revision	= 5,
		.match		= set_match_v5,
		.matchsize	= sizeof(struct xt_set_info_match_v5),
		.checkentry	= set_match_v5_checkentry,
		.destroy	= set_match_v5_destroy,
		.me		= THIS_MODULE
	},
};

static struct xt_target set_targets[] __read_mostly = {
	{
		.name		= "SET",
		.revision	= 0,
		.family		= NFPROTO_IPV4,
		.target		= set_target_v0,
		.targetsize	= sizeof(struct xt_set_info_target_v0),
		.checkentry	= set_target_v0_checkentry,
		.destroy	= set_target_v0_destroy,
		.me		= THIS_MODULE
	},
	{
		.name		= "SET",
		.revision	= 1,
		.family		= NFPROTO_IPV4,
		.target		= set_target_v1,
		.targetsize	= sizeof(struct xt_set_info_target_v1),
		.checkentry	= set_target_v1_checkentry,
		.destroy	= set_target_v1_destroy,
		.me		= THIS_MODULE
	},
	{
		.name		= "SET",
		.revision	= 1,
		.family		= NFPROTO_IPV6,
		.target		= set_target_v1,
		.targetsize	= sizeof(struct xt_set_info_target_v1),
		.checkentry	= set_target_v1_checkentry,
		.destroy	= set_target_v1_destroy,
		.me		= THIS_MODULE
	},
	/* --timeout and --exist flags support */
	{
		.name		= "SET",
		.revision	= 2,
		.family		= NFPROTO_IPV4,
		.target		= set_target_v2,
		.targetsize	= sizeof(struct xt_set_info_target_v2),
		.checkentry	= set_target_v2_checkentry,
		.destroy	= set_target_v2_destroy,
		.me		= THIS_MODULE
	},
	{
		.name		= "SET",
		.revision	= 2,
		.family		= NFPROTO_IPV6,
		.target		= set_target_v2,
		.targetsize	= sizeof(struct xt_set_info_target_v2),
		.checkentry	= set_target_v2_checkentry,
		.destroy	= set_target_v2_destroy,
		.me		= THIS_MODULE
	},
	/* --map-set support */
	{
		.name		= "SET",
		.revision	= 3,
		.family		= NFPROTO_IPV4,
		.target		= set_target_v3,
		.targetsize	= sizeof(struct xt_set_info_target_v3),
		.checkentry	= set_target_v3_checkentry,
		.destroy	= set_target_v3_destroy,
		.me		= THIS_MODULE
	},
	{
		.name		= "SET",
		.revision	= 3,
		.family		= NFPROTO_IPV6,
		.target		= set_target_v3,
		.targetsize	= sizeof(struct xt_set_info_target_v3),
		.checkentry	= set_target_v3_checkentry,
		.destroy	= set_target_v3_destroy,
		.me		= THIS_MODULE
	},
	/* proper support for "physdev:" prefix */
	{
		.name		= "SET",
		.revision	= 4,
		.family		= NFPROTO_IPV4,
		.target		= set_target_v4,
		.targetsize	= sizeof(struct xt_set_info_target_v4),
		.checkentry	= set_target_v4_checkentry,
		.destroy	= set_target_v4_destroy,
		.me		= THIS_MODULE
	},
	{
		.name		= "SET",
		.revision	= 4,
		.family		= NFPROTO_IPV6,
		.target		= set_target_v4,
		.targetsize	= sizeof(struct xt_set_info_target_v4),
		.checkentry	= set_target_v4_checkentry,
		.destroy	= set_target_v4_destroy,
		.me		= THIS_MODULE
	},
};

static int __init xt_set_init(void)
{
	int ret = xt_register_matches(set_matches, ARRAY_SIZE(set_matches));

	if (!ret) {
		ret = xt_register_targets(set_targets,
					  ARRAY_SIZE(set_targets));
		if (ret)
			xt_unregister_matches(set_matches,
					      ARRAY_SIZE(set_matches));
	}
	return ret;
}

static void __exit xt_set_fini(void)
{
	xt_unregister_matches(set_matches, ARRAY_SIZE(set_matches));
	xt_unregister_targets(set_targets, ARRAY_SIZE(set_targets));
}

module_init(xt_set_init);
module_exit(xt_set_fini);
