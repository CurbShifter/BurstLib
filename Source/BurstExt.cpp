/*
Copyright (C) 2018  CurbShifter

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "BurstExt.h"


BurstExt::BurstExt(String hostUrl) : BurstKit(hostUrl)
{
	nodeHopCount = 0;
	maxHopNodeRequests = 0;
}

BurstExt::~BurstExt()
{
}

String BurstExt::broadcastTransaction(String signedTransactionBytesStrHex)
{
	NodeHop(false);
	return BurstKit::broadcastTransaction(signedTransactionBytesStrHex);
}

String BurstExt::GetUrlStr(const String url)
{
	NodeHop(false);
	return BurstKit::GetUrlStr(url);
}

void BurstExt::EnableNodeHop(unsigned int maxHopNodeRequests)
{
	this->maxHopNodeRequests = maxHopNodeRequests;
}

unsigned int BurstExt::GetNodeCount()
{
	return api_peers_whitelist.size();
}

String BurstExt::NodeHop(const bool force)
{
	if (maxHopNodeRequests > 0 || force)
	{
		int port = 8125;
		String hopNode;
		if (nodeHopCount >= maxHopNodeRequests)
		{
			nodeHopCount = 0;
			
			String root_peers = URL(GetNode() + "burst?requestType=getPeers&state=CONNECTED").readEntireTextStream();
			var root_peersJson;
			Result r = JSON::parse(root_peers, root_peersJson);

			String hostStripped = GetNode();
			hostStripped = hostStripped.substring(0, hostStripped.length() - 1);

			if (hostStripped.getTrailingIntValue() == 6876) // check if testnet
				port = 6876;
			
			// if it has an response with peers, pick a random peer and check if it is API accessible
			if (root_peersJson["peers"].isArray())
			{
				Array<int> jumble;
				for (int i = 0; i < root_peersJson["peers"].size(); i++)
				{ // randomize the read index
					Random random;
					int index = random.nextInt(jumble.size() + 1);
					jumble.insert(index, i);
				}
				for (int i = 0; i < jumble.size() && hopNode.isEmpty(); i++)
				{
					String peer = root_peersJson["peers"][jumble[i]].toString() + ":" + String(port);
					if (api_peers_whitelist.contains(peer) == false &&
						api_peers_blacklist.contains(peer) == false)
					{
						String url((GetForceSSL_TSL() ? "https://" : "http://") + peer + "/burst?requestType=getPeers&state=CONNECTED");
						int64 startTimeMs = Time::currentTimeMillis();
						String peers = URL(url).readEntireTextStream();
						if (Time::currentTimeMillis() - startTimeMs < 100) // at least 0.1 sec response time
						{
							var peersJson;
							Result r = JSON::parse(peers, peersJson);
							if (peersJson["peers"].isArray() && 
								peersJson["peers"].size() > 2)
							{ // make sure its not a dead end and has more peers
								api_peers_whitelist.add(peer);
								hopNode = peer;
							}
							else api_peers_blacklist.add(peer);
						}
						else api_peers_blacklist.add(peer);
					}
				}
				if (hopNode.isEmpty() && api_peers_whitelist.size() > 0)
				{ // no peer found. get one from our api_peers_whitelist
					Random random;
					int index = random.nextInt(api_peers_whitelist.size() + 1);
					hopNode = api_peers_whitelist[index];
				}
			}
		}
		else nodeHopCount++;

		if (hopNode.isNotEmpty())
		{
			String r((GetForceSSL_TSL() ? "https://" : "http://") + hopNode + "/");
			SetNode(r);
			return r;
		}
	}
	return String::empty;
}