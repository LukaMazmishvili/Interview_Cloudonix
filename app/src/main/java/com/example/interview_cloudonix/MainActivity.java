package com.example.interview_cloudonix;

import android.annotation.SuppressLint;
import android.os.Bundle;
import android.view.View;
import android.widget.ImageView;
import android.widget.ProgressBar;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import com.example.interview_cloudonix.databinding.ActivityMainBinding;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.IOException;
import java.util.concurrent.TimeUnit;

import okhttp3.Call;
import okhttp3.Callback;
import okhttp3.MediaType;
import okhttp3.OkHttpClient;
import okhttp3.Request;
import okhttp3.RequestBody;
import okhttp3.Response;

@SuppressLint("SetTextI18n")
public class MainActivity extends AppCompatActivity {

    static {
        System.loadLibrary("interview_cloudonix");
    }

    private ActivityMainBinding binding;

    private TextView textView;
    private ImageView statusIcon;
    private ProgressBar progressBar;

    private static final String URL = "https://s7om3fdgbt7lcvqdnxitjmtiim0uczux.lambda-url.us-east-2.on.aws/";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        textView = binding.sampleText;
        statusIcon = binding.statusIcon;
        progressBar = binding.progressBar;

        binding.button.setOnClickListener(v -> fetchAndSendIp());
    }

    private void fetchAndSendIp() {
        String ipAddress = getDeviceIp();
        textView.setText(ipAddress);
        sendIpToServer(ipAddress);
    }

    private void sendIpToServer(String ipAddress) {
        progressBar.setVisibility(View.VISIBLE);

        OkHttpClient client = new OkHttpClient.Builder()
                .connectTimeout(3, TimeUnit.SECONDS)
                .build();

        JSONObject json = new JSONObject();
        try {
            json.put("address", ipAddress);
        } catch (JSONException e) {
            e.printStackTrace();
        }

        RequestBody body = RequestBody.create(json.toString(), MediaType.get("application/json; charset=utf-8"));
        Request request = new Request.Builder()
                .url(URL)
                .post(body)
                .build();

        client.newCall(request).enqueue(new Callback() {
            @Override
            public void onFailure(@NonNull Call call, @NonNull IOException e) {
                runOnUiThread(() -> {
                    progressBar.setVisibility(View.GONE);
                    textView.setText("Request failed: " + e.getMessage());
                    statusIcon.setImageResource(R.drawable.red_icon);
                });
            }

            @Override
            public void onResponse(@NonNull Call call, @NonNull Response response) throws IOException {
                assert response.body() != null;
                String responseData = response.body().string();
                runOnUiThread(() -> {
                    progressBar.setVisibility(View.GONE);
                    try {
                        JSONObject jsonResponse = new JSONObject(responseData);
                        boolean nat = jsonResponse.getBoolean("nat");
                        statusIcon.setImageResource(nat ? R.drawable.green_icon : R.drawable.red_icon);
                    } catch (JSONException e) {
                        e.printStackTrace();
                        textView.setText("Invalid response");
                    }
                });
            }
        });
    }

    public native String getDeviceIp();
}