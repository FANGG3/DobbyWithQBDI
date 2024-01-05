package com.f.qbdi;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.util.Log;
import android.widget.TextView;

import com.f.nativelib.NativeLib;
import com.f.qbdi.databinding.ActivityMainBinding;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'qbdi' library on application startup.
    static {
        System.loadLibrary("nativelib");

    }

    private ActivityMainBinding binding;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        new NativeLib().stringFromJNI();
        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        // Example of a call to a native method
        TextView tv = binding.sampleText;
        System.loadLibrary("test");
        tv.setText(stringFromJNI());
        Log.i("qbdi", String.valueOf(add(1,3)));


    }

    /**
     * A native method that is implemented by the 'qbdi' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();
    public native int add(int a, int b);

}