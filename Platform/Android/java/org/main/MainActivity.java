package org.main;

import android.app.Activity;
import android.os.Bundle;

import org.main.networking.Networking;

public class MainActivity extends Activity {

    private Networking networking;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        networking = new Networking(this);
        networking.discoverPeers();
    }
}