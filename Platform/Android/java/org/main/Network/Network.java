package org.main.networking;

import android.content.Context;
import android.net.wifi.p2p.WifiP2pManager;
import android.util.Log;

public class Networking {

    private WifiP2pManager manager;
    private WifiP2pManager.Channel channel;

    public Networking(Context context) {
        manager = (WifiP2pManager) context.getSystemService(Context.WIFI_P2P_SERVICE);

        channel = manager.initialize(
                context,
                context.getMainLooper(),
                null
        );
    }

    public void discoverPeers() {
        manager.discoverPeers(channel, new WifiP2pManager.ActionListener() {
            @Override
            public void onSuccess() {
                Log.d("MyApp", "Peer discovery started");
            }

            @Override
            public void onFailure(int reason) {
                Log.d("MyApp", "Peer discovery failed: " + reason);
            }
        });
    }

    public WifiP2pManager getManager() {
        return manager;
    }

    public WifiP2pManager.Channel getChannel() {
        return channel;
    }
}