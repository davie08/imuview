package com.toynext.imuview;

import android.content.ComponentName;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.util.Log;

import com.google.android.material.bottomnavigation.BottomNavigationView;

import androidx.appcompat.app.AppCompatActivity;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentManager;
import androidx.navigation.NavController;
import androidx.navigation.Navigation;
import androidx.navigation.fragment.NavHostFragment;
import androidx.navigation.ui.AppBarConfiguration;
import androidx.navigation.ui.NavigationUI;

import com.toynext.imuview.databinding.ActivityMainBinding;
import com.toynext.imuview.ui.home.HomeFragment;

import java.util.List;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'trimu' library on application startup.
    static {
        System.loadLibrary("imuview");
    }

    private ActivityMainBinding binding;
    public BluetoothLeService mBluetoothLeService;

    private static final String TAG = "IMU-MAIN";
    private static final String BT_DEVICE_ADDR = "54:6C:0E:B9:1A:52";

    // Code to manage Service lifecycle.
    private final ServiceConnection mServiceConnection = new ServiceConnection() {

        @Override
        public void onServiceConnected(ComponentName componentName, IBinder service) {
            mBluetoothLeService = ((BluetoothLeService.LocalBinder) service).getService();
            if (!mBluetoothLeService.initialize()) {
                Log.e(TAG, "Unable to initialize Bluetooth");
                finish();
            }

            mBluetoothLeService.setActivity(activity);
            // Automatically connects to the device upon successful start-up initialization.
            mBluetoothLeService.connect(BT_DEVICE_ADDR);
        }

        @Override
        public void onServiceDisconnected(ComponentName componentName) {
            mBluetoothLeService = null;
        }
    };

    MainActivity activity;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        activity = this;

        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        // Passing each menu ID as a set of Ids because each
        // menu should be considered as top level destinations.
        AppBarConfiguration appBarConfiguration = new AppBarConfiguration.Builder(
                R.id.navigation_home, R.id.navigation_dashboard, R.id.navigation_notifications)
                .build();
        NavController navController = Navigation.findNavController(this, R.id.nav_host_fragment_activity_main);
        NavigationUI.setupActionBarWithNavController(this, navController, appBarConfiguration);
        NavigationUI.setupWithNavController(binding.navView, navController);

        Intent gattServiceIntent = new Intent(this, BluetoothLeService.class);
        //绑定服务
        bindService(gattServiceIntent, mServiceConnection, BIND_AUTO_CREATE);

        Log.i(TAG, getVersion());
    }

    public Fragment getFragment(Class<?> clazz) {
        List<Fragment> fragments = getSupportFragmentManager().getFragments();
        if (fragments.size() > 0) {
            NavHostFragment navHostFragment = (NavHostFragment) fragments.get(0);
            List<Fragment> childfragments = navHostFragment.getChildFragmentManager().getFragments();
            if(childfragments.size() > 0){
                for (int j = 0; j < childfragments.size(); j++) {
                    Fragment fragment = childfragments.get(j);
                    if(fragment.getClass().isAssignableFrom(clazz)){
                        Log.i(TAG, "HomeFragment: " + fragment);
                        return fragment;
                    }
                }
            }
        }
        return null;
    }

    private HomeFragment homeFragment;

    public HomeFragment GetHomeFragment() {
        return homeFragment;
    }

    @Override
    protected void onResume() {
        super.onResume();

        homeFragment = (HomeFragment) getFragment(HomeFragment.class);

        if (mBluetoothLeService != null) {
            final boolean result = mBluetoothLeService.connect(BT_DEVICE_ADDR);
            Log.d(TAG, "Connect request result=" + result);
        }
    }
    @Override
    protected void onDestroy() {
        super.onDestroy();
        //解除服务的绑定
        unbindService(mServiceConnection);
        mBluetoothLeService = null;
    }

    public native String getVersion();
}