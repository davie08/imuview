package com.toynext.imuview.ui.dashboard;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.graphics.Paint;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.content.ContextCompat;
import androidx.fragment.app.Fragment;
import androidx.lifecycle.Observer;
import androidx.lifecycle.ViewModelProvider;

import com.github.mikephil.charting.charts.LineChart;
import com.github.mikephil.charting.components.Description;
import com.toynext.imuview.BluetoothLeService;
import com.toynext.imuview.MainActivity;
import com.toynext.imuview.R;
import com.toynext.imuview.databinding.FragmentDashboardBinding;

public class DashboardFragment extends Fragment {

    private final int MESSAGE_UPDATE = 1;

    private DashboardViewModel dashboardViewModel;
    private FragmentDashboardBinding binding;
    private LineChart mLineChart = null;
    private RawAccelChart mAccelChart = null;
    private Context context;

    private int [] raw_data = {0, 0, 0, 0, 0, 0};


    private final BroadcastReceiver mGattUpdateReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            final String action = intent.getAction();
            if (BluetoothLeService.ACTION_DATA_AVAILABLE.equals(action)) {
                postData(intent.getIntArrayExtra(BluetoothLeService.EXTRA_DATA));
            }
        }
    };

    private void postData(int[] data) {
        Log.d("ABC", "df recv " + data.length + " bytes");
        update(data);
    }

    public View onCreateView(@NonNull LayoutInflater inflater,
                             ViewGroup container, Bundle savedInstanceState) {
        this.context = getActivity();
        dashboardViewModel =
                new ViewModelProvider(this).get(DashboardViewModel.class);

        binding = FragmentDashboardBinding.inflate(inflater, container, false);
        View root = binding.getRoot();

        final TextView textView = binding.textDashboard;
        dashboardViewModel.getText().observe(getViewLifecycleOwner(), new Observer<String>() {
            @Override
            public void onChanged(@Nullable String s) {
                textView.setText(s);
            }
        });

        mLineChart = binding.multiLineGlChart;
        mLineChart.setKeepScreenOn(true);

        MainActivity act = (MainActivity) getActivity();
        mAccelChart = new RawAccelChart();
        mAccelChart.PrepareLineChart(mLineChart);

        return root;
    }

    @Override
    public void onResume() {
        super.onResume();
        context.registerReceiver(mGattUpdateReceiver, makeGattUpdateIntentFilter());
    }
    @Override
    public void onDestroyView() {
        super.onDestroyView();
        context.unregisterReceiver(mGattUpdateReceiver);
        binding = null;
    }

    private final Handler mConnHandler = new Handler() {
        @Override
        public void handleMessage(final Message msg) {
            switch (msg.what) {
                case MESSAGE_UPDATE:
                    if(mAccelChart != null) {
                        mAccelChart.AddAccelEntry(raw_data[0] * 0.488f,
                                raw_data[1] * 0.488f,
                                raw_data[2] * 0.488f);
                    }
                    break;
            }
            super.handleMessage(msg);
        }
    };

    public void update(int[] data)
    {
        raw_data = data;
        Message msg = new Message();
        msg.what = MESSAGE_UPDATE;
        mConnHandler.sendMessage(msg);

    }

    private static IntentFilter makeGattUpdateIntentFilter() {
        final IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(BluetoothLeService.ACTION_GATT_CONNECTED);
        intentFilter.addAction(BluetoothLeService.ACTION_GATT_DISCONNECTED);
        intentFilter.addAction(BluetoothLeService.ACTION_GATT_SERVICES_DISCOVERED);
        intentFilter.addAction(BluetoothLeService.ACTION_DATA_AVAILABLE);
        return intentFilter;
    }
}