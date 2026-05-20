package org.main;

import android.util.Log;
import android.app.Activity;
import android.os.Bundle;

public class MainActivity extends Activity {

    static {
        System.loadLibrary("MyGame");
    }

    public native void nativeInit();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        Log.e("MyApp", "JAVA BEFORE JNI");

        nativeInit();

        Log.e("MyApp", "JAVA AFTER JNI");
    }
}