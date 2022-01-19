package com.toynext.imuview;

import android.app.Activity;
import android.app.Service;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothProfile;
import android.content.Context;
import android.content.Intent;
import android.os.Binder;
import android.os.IBinder;
import android.util.Log;

import androidx.annotation.Nullable;

import com.toynext.imuview.ui.dashboard.DashboardFragment;

import java.util.List;
import java.util.UUID;

public class BluetoothLeService extends Service {
    private final static String TAG = BluetoothLeService.class.getSimpleName();

    private BluetoothManager mBluetoothManager;
    private BluetoothAdapter mBluetoothAdapter;
    private String mBluetoothDeviceAddress;
    private BluetoothGatt mBluetoothGatt;
    private MainActivity mMainActivity;
    public int mConnectionState = STATE_DISCONNECTED;

    public static final int STATE_DISCONNECTED = 0;
    public static final int STATE_CONNECTING = 1;
    public static final int STATE_CONNECTED = 2;

    //用于标识广播的字符常量
    public final static String ACTION_GATT_CONNECTED =
            "com.toynext.imuview.ACTION_GATT_CONNECTED";
    public final static String ACTION_GATT_DISCONNECTED =
            "com.toynext.imuview.ACTION_GATT_DISCONNECTED";
    public final static String ACTION_GATT_SERVICES_DISCOVERED =
            "com.toynext.imuviewo.ACTION_GATT_SERVICES_DISCOVERED";
    public final static String ACTION_DATA_AVAILABLE =
            "com.toynext.imuview.ACTION_DATA_AVAILABLE";
    public final static String EXTRA_DATA =
            "com.toynext.imuview.EXTRA_DATA";

    private static final UUID mUUID = UUID.fromString("0000fff1-0000-1000-8000-00805f9b34fb");

    public static final UUID batteryUUID = UUID.fromString("0000fff2-0000-1000-8000-00805f9b34fb");
    public static final UUID cmdUUID = UUID.fromString("0000fff3-0000-1000-8000-00805f9b34fb");
    public static final UUID dataUUID = UUID.fromString("0000fff4-0000-1000-8000-00805f9b34fb");

    public boolean sendBluetooth(UUID uuid, byte[] cmd) {
        BluetoothGattCharacteristic targetCharacteristic = getService(uuid);

        if (targetCharacteristic == null) {
            Log.e(TAG,"sendBluetooth no service!");
            return false;
        }

        if (!targetCharacteristic.setValue(cmd)) {
            Log.e(TAG,"sendBluetooth setValue false!");
            return false;
        }

        targetCharacteristic.setWriteType(BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE);

        if (!mBluetoothGatt.writeCharacteristic(targetCharacteristic)) {
            Log.e(TAG,"sendBluetooth writeCharacteristic false!");
            return false;
        }

        Log.e(TAG, "sendBluetooth ok");
        return true;
    }

    public boolean getBluetooth(UUID uuid) {
        BluetoothGattCharacteristic targetCharacteristic = getService(uuid);

        if (targetCharacteristic == null) {
            Log.e(TAG,"getBluetooth no service!");
            return false;
        }

        if (!mBluetoothGatt.readCharacteristic(targetCharacteristic)) {
            Log.e(TAG,"getBluetooth readCharacteristic fail");
            return false;
        }

        Log.e(TAG,"getBluetooth ok");

        return true;
    }

    public void setActivity(MainActivity act) {
        Log.e(TAG, "setActivity");
        mMainActivity = act;
    }

    public boolean setDataEnable(boolean enable) {
        BluetoothGattCharacteristic targetCharacteristic = getService(mUUID);
        if (targetCharacteristic == null) {
            Log.e(TAG,"logging no service!");
            return false;
        }

        if ((BluetoothGattCharacteristic.PROPERTY_NOTIFY) > 0) {
            setCharacteristicNotification(
                    targetCharacteristic, enable);//通知安卓系统我要接受characristic这个特征的数据，true为接受，false为不接受

            BluetoothGattDescriptor descriptor = targetCharacteristic.getDescriptor(
                    UUID.fromString("00002902-0000-1000-8000-00805f9b34fb")
            );
            descriptor.setValue(enable?BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE:BluetoothGattDescriptor.DISABLE_NOTIFICATION_VALUE);
            writeDescriptor(descriptor);
        }
        Log.i(TAG, "set recv data is " + enable);
        return true;
    }

    public void CalibrationSensor() {
        Calibration(10);
    }

    // Implements callback methods for GATT events that the app cares about.  For example,
    // connection change and services discovered.
    private final BluetoothGattCallback mGattCallback = new BluetoothGattCallback() {
        @Override
        public void onConnectionStateChange(BluetoothGatt gatt, int status, int newState) {
            if (newState == BluetoothProfile.STATE_CONNECTED) {
                mConnectionState = STATE_CONNECTED;
                mBluetoothGatt.discoverServices();
                Log.i(TAG, "连接上了GATT服务器，Connected to GATT server.");
            } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
                mConnectionState = STATE_DISCONNECTED;
                Log.i(TAG, "不能从GATT服务器连接，Disconnected from GATT server.");
            }
        }

        @Override
        public void onServicesDiscovered(BluetoothGatt gatt, int status) {
            if (status == BluetoothGatt.GATT_SUCCESS) {
                Log.w(TAG,"onServicesDiscovered被系统回调,并广播出去");

                getBluetooth(batteryUUID);
            } else {
                Log.w(TAG, "onServicesDiscovered被系统回调,没有发现服务，不广播，status= " + status);
            }
        }

        @Override
        public void onCharacteristicRead(BluetoothGatt gatt,
                                         BluetoothGattCharacteristic characteristic,
                                         int status) {
            if (status == BluetoothGatt.GATT_SUCCESS) {

                if (characteristic.getUuid().equals(batteryUUID)) {
                    byte[] battery = characteristic.getValue();
                    mMainActivity.GetHomeFragment().SetString(String.valueOf(battery[0]));
                    return;
                }

                final byte[] data = characteristic.getValue();
                if (data != null && data.length > 0) {
                    int ret = parseData(data);
                    broadcastUpdate(ACTION_DATA_AVAILABLE, ReadIMU());
                }
            }
        }

        @Override
        public void onCharacteristicChanged(BluetoothGatt gatt,
                                            BluetoothGattCharacteristic characteristic) {
            final byte[] data = characteristic.getValue();
            if (data != null && data.length > 0) {
                int ret = parseData(data);

                if (ret == 1)
                    mMainActivity.GetHomeFragment().SetString("RIGHT");
                else if (ret == 2)
                    mMainActivity.GetHomeFragment().SetString("LEFT");
                else if (ret == 8)
                    mMainActivity.GetHomeFragment().SetString("CLICKED");

                broadcastUpdate(ACTION_DATA_AVAILABLE, ReadIMU());
            }
        }

        //当BluetoothGatt.writeCharacteristic(BluetoothGattCharacteristic)执行时,系统异步调用该回调方法
        @Override
        public void onCharacteristicWrite(BluetoothGatt gatt,BluetoothGattCharacteristic characteristic,int status){
            super.onCharacteristicWrite(gatt,characteristic,status);
            if(status==BluetoothGatt.GATT_SUCCESS) {
            }
        }
    };

    private void broadcastUpdate(final String action,
                                 final int[] data) {
        final Intent intent = new Intent(action);
        // 解析数据，即将字节转换成字符串
        if (data != null && data.length > 0) {
            intent.putExtra(EXTRA_DATA, data);
        }
        sendBroadcast(intent);
    }

    //创建本地Binder,将该类实例传递给DeviceControlActivity
    public class LocalBinder extends Binder {
        BluetoothLeService getService() {
            return BluetoothLeService.this;
        }
    }

    @Nullable
    @Override
    public IBinder onBind(Intent intent) {
        return mBinder;
    }

    //系统回调该方法时，即关闭BluetoothGatt服务
    @Override
    public boolean onUnbind(Intent intent){
        // After using a given device, you should make sure that BluetoothGatt.close() is called
        // such that resources are cleaned up properly.  In this particular example, close() is
        // invoked when the UI is disconnected from the Service.
        // close();
        IMUFreeData();
        return super.onUnbind(intent);
    }

    private final IBinder mBinder=new LocalBinder();

    /**
     * Initializes a reference to the local Bluetooth adapter.
     *
     * @return Return true if the initialization is successful.
     */
    public boolean initialize() {
        // For API level 18 and above, get a reference to BluetoothAdapter through
        // BluetoothManager.
        if (mBluetoothManager == null) {
            mBluetoothManager = (BluetoothManager) getSystemService(Context.BLUETOOTH_SERVICE);
            if (mBluetoothManager == null) {
                Log.e(TAG, "Unable to initialize BluetoothManager.");
                return false;
            }
        }

        mBluetoothAdapter = mBluetoothManager.getAdapter();
        if (mBluetoothAdapter == null) {
            Log.e(TAG, "Unable to obtain a BluetoothAdapter.");
            return false;
        }

        IMUInitData();
        return true;
    }

    /**
     * Connects to the GATT server hosted on the Bluetooth LE device.
     *
     * @param address The device address of the destination device.
     *
     * @return Return true if the connection is initiated successfully. The connection result
     *         is reported asynchronously through the
     *         {@code BluetoothGattCallback#onConnectionStateChange(android.bluetooth.BluetoothGatt, int, int)}
     *         callback.
     */
    public boolean connect(final String address) {
        if (mBluetoothAdapter == null || address == null) {
            Log.w(TAG, "连接失败，BluetoothAdapter not initialized or unspecified address.");
            return false;
        }

        // Previously connected device.  Try to reconnect.
        if (mBluetoothDeviceAddress != null && address.equals(mBluetoothDeviceAddress)
                && mBluetoothGatt != null) {
            Log.d(TAG, "连接存在的设备：Trying to use an existing mBluetoothGatt for connection.");
            if (mBluetoothGatt.connect()) {
                mConnectionState = STATE_CONNECTING;
                return true;
            } else {
                return false;
            }
        }

        final BluetoothDevice device = mBluetoothAdapter.getRemoteDevice(address.toUpperCase());
        if (device == null) {
            Log.w(TAG, "没有找到设备，Device not found.  Unable to connect.");
            return false;
        }
        // We want to directly connect to the device, so we are setting the autoConnect
        // parameter to false.
        mBluetoothGatt = device.connectGatt(this, false, mGattCallback);//连接时，此处得到BluetoothGatt实例
        Log.d(TAG, "Trying to create a new connection.");
        mBluetoothDeviceAddress = address;//在连接当中传入设备地址到BluetoothService类中
        mConnectionState = STATE_CONNECTING;
        return true;
    }

    /**
     * Disconnects an existing connection or cancel a pending connection. The disconnection result
     * is reported asynchronously through the
     * {@code BluetoothGattCallback#onConnectionStateChange(android.bluetooth.BluetoothGatt, int, int)}
     * callback.
     */
    public void disconnect() {
        if (mBluetoothAdapter == null || mBluetoothGatt == null) {
            Log.w(TAG, "蓝牙适配没有初始化成功，BluetoothAdapter not initialized");
            return;
        }
        mBluetoothGatt.disconnect();
    }

    /**
     * After using a given BLE device, the app must call this method to ensure resources are
     * released properly.
     */
    public void close() {
        if (mBluetoothGatt == null) {
            return;
        }
        mBluetoothGatt.close();
        mBluetoothGatt = null;
    }

    /**
     * Request a read on a given {@code BluetoothGattCharacteristic}. The read result is reported
     * asynchronously through the {@code BluetoothGattCallback#onCharacteristicRead(android.bluetooth.BluetoothGatt, android.bluetooth.BluetoothGattCharacteristic, int)}
     * callback.
     *
     * @param characteristic The characteristic to read from.
     */
    public void readCharacteristic(BluetoothGattCharacteristic characteristic) {
        if (mBluetoothAdapter == null || mBluetoothGatt == null) {
            Log.w(TAG, "BluetoothAdapter not initialized");
            return;
        }
        mBluetoothGatt.readCharacteristic(characteristic);
    }

    public void writeDescriptor(BluetoothGattDescriptor descriptor) {
        if (mBluetoothAdapter == null || mBluetoothGatt == null) {
            Log.w(TAG, "BluetoothAdapter not initialized");
            return;
        }
        mBluetoothGatt.writeDescriptor(descriptor);
    }

    public boolean writeCharacteristic(final BluetoothGattCharacteristic characteristic) {
        final BluetoothGatt gatt = mBluetoothGatt;
        if (gatt == null || characteristic == null)
            return false;
        // Check characteristic property
        final int properties = characteristic.getProperties();
        if ((properties & (BluetoothGattCharacteristic.PROPERTY_WRITE | BluetoothGattCharacteristic.PROPERTY_WRITE_NO_RESPONSE)) == 0)
            return false;

        return gatt.writeCharacteristic(characteristic);
    }

    public BluetoothGattCharacteristic getService(UUID uuid) {
        BluetoothGattCharacteristic targetCharacteristic = null;
        for (BluetoothGattService bluetoothGattService : getSupportedGattServices()) {
            for (BluetoothGattCharacteristic bluetoothGattCharacteristic : bluetoothGattService.getCharacteristics()) {
                if (bluetoothGattCharacteristic.getUuid().toString().equals(uuid.toString())) {
                    targetCharacteristic = bluetoothGattCharacteristic;
                }
            }
        }

        return targetCharacteristic;
    }
    public boolean writeBle(byte[] data, UUID uuid) {
        BluetoothGattCharacteristic targetCharacteristic = getService(uuid);
        if (targetCharacteristic == null){
            return false;
        }else {
            targetCharacteristic.setValue(data);
            return writeCharacteristic(targetCharacteristic);
        }
    }

    /**
     * Enables or disables notification on a give characteristic.
     *
     * @param characteristic Characteristic to act on.
     * @param enabled If true, enable notification.  False otherwise.
     */
    //通知安卓系统我要接受characristic这个制定特征的数据，true为接受，false为不接受
    public void setCharacteristicNotification(BluetoothGattCharacteristic characteristic,
                                              boolean enabled) {
        if (mBluetoothAdapter == null || mBluetoothGatt == null) {
            Log.w(TAG, "BluetoothAdapter not initialized");
            return;
        }
        mBluetoothGatt.setCharacteristicNotification(characteristic, enabled);
    }

    /**
     * Retrieves a list of supported GATT services on the connected device. This should be
     * invoked only after {@code BluetoothGatt#discoverServices()} completes successfully.
     *
     * @return A {@code List} of supported services.
     */
    public List<BluetoothGattService> getSupportedGattServices() {
        if (mBluetoothGatt == null) return null;

        return mBluetoothGatt.getServices();
    }


    public native int parseData(byte []data);
    public native void Calibration(int delay_sec);
    public native void IMUInitData();
    public native void IMUFreeData();
    public native int[] ReadIMU();
}