package com.toynext.imuview.ui.home;

import android.graphics.Color;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.Switch;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.lifecycle.Observer;
import androidx.lifecycle.ViewModelProvider;

import com.toynext.imuview.MainActivity;
import com.toynext.imuview.R;
import com.toynext.imuview.databinding.FragmentHomeBinding;

public class HomeFragment extends Fragment {

    private HomeViewModel homeViewModel;
    private FragmentHomeBinding binding;
    private static final String TAG = "IMU-HOME";

    public View onCreateView(@NonNull LayoutInflater inflater,
                             ViewGroup container, Bundle savedInstanceState) {
        homeViewModel =
                new ViewModelProvider(this).get(HomeViewModel.class);

        binding = FragmentHomeBinding.inflate(inflater, container, false);
        View root = binding.getRoot();

        final TextView textView = binding.textHome;
        textView.setTextSize(100);
        textView.setTextColor(Color.rgb(255, 0, 0));
        homeViewModel.getText().observe(getViewLifecycleOwner(), new Observer<String>() {
            @Override
            public void onChanged(@Nullable String s) {
                textView.setText(s);
            }
        });

        return root;
    }

    final Handler handler = new Handler(Looper.getMainLooper()){
        @Override
        public void handleMessage(Message msg){
            super.handleMessage(msg);
            if(msg.what == 1){
                homeViewModel.SetString((String)msg.obj);
            }
        }
    };

    public void SetString(String str) {
        Message message = new Message();
        message.what = 1;
        message.obj = str;
        handler.sendMessage(message);
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        binding = null;
    }

    @Override
    public void onResume() {
        super.onResume();
        MainActivity act = (MainActivity) getActivity();
        final Switch aSwitch = binding.recvEnable;
        final Button aIMUCalibration = binding.IMUCalibration;

        if (aSwitch != null) {
            aSwitch.setChecked(false);
            aSwitch.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
                @Override
                public void onCheckedChanged(CompoundButton compoundButton, boolean b) {
                    //控制开关字体颜色
                    if (act.mBluetoothLeService.setDataEnable(b) == false) {
                        aSwitch.setChecked(false);
                        return ;
                    }
                    if (aIMUCalibration != null) {
                        aIMUCalibration.setEnabled(b);
                    }
                }

            });
        }

        if (aIMUCalibration != null) {
            aIMUCalibration.setEnabled(false);
            aIMUCalibration.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    aSwitch.setEnabled(false);
                    act.mBluetoothLeService.CalibrationSensor();
                    aSwitch.setEnabled(true);
                }
            });
        }
    }
}