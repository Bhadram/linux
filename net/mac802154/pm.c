#include <net/mac802154.h>
#include <net/rtnetlink.h>

#include "ieee802154_i.h"
#include "driver-ops.h"

int __ieee802154_suspend(struct ieee802154_hw *hw)
{
	struct ieee802154_local *local = hw_to_local(hw);
	struct ieee802154_sub_if_data *sdata;

	if (!local->open_count)
		goto suspend;

	ieee802154_stop_queue(hw);

	flush_workqueue(local->workqueue);

	/* remove all interfaces that were created in the driver */
	list_for_each_entry(sdata, &local->interfaces, list) {
		if (!ieee802154_sdata_running(sdata))
			continue;

		ieee802154_remove_interfaces(local);
	}

	/* Stop hardware - this must stop RX */
	if (local->open_count)
		drv_stop(local);

suspend:
	return 0;
}

int __ieee802154_resume(struct ieee802154_hw *hw)
{
	struct ieee802154_local *local = hw_to_local(hw);
	struct ieee802154_sub_if_data *sdata;
	struct net_device *ndev = NULL;
	int res;

	/* nothing to do if HW shouldn't run */
	if (!local->open_count)
		goto wake_up;

	/* restart hardware */
	res = drv_start(local);
	if (res)
		return res;

	/* add interfaces */
	list_for_each_entry(sdata, &local->interfaces, list) {
		if (sdata->wpan_dev.iftype != NL802154_IFTYPE_MONITOR &&
		    ieee802154_sdata_running(sdata)) {
			ndev = ieee802154_if_add(local, "wpan%d", NET_NAME_ENUM,
						 sdata->wpan_dev.iftype,
						 sdata->wpan_dev.extended_addr);
			if (WARN_ON(ndev))
				break;
		}
	}
wake_up:
	ieee802154_wake_queue(hw);

	return 0;
}

