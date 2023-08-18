package com.yeungeek.opengltutorial

import android.content.Context
import android.content.Intent
import android.os.Build
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import com.yeungeek.opengltutorial.data.Sample
import com.yeungeek.opengltutorial.ui.components.SampleDetailScreen
import com.yeungeek.opengltutorial.ui.theme.OpenGLTutorialTheme
import java.io.Serializable

class ShaderDetailActivity : ComponentActivity() {
    private val sample: Sample by lazy {
        getSerializable(intent, KEY_SAMPLE, Sample::class.java)
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContent {
            OpenGLTutorialTheme {
                SampleDetailScreen(sample) {
                    finish()
                }
            }
        }
    }

    companion object {
        private const val KEY_SAMPLE = "key_sample"

        fun newIntent(context: Context, sample: Sample) =
            Intent(context, ShaderDetailActivity::class.java).apply {
                putExtra(KEY_SAMPLE, sample)
            }
    }

    @Suppress("DEPRECATION")
    private fun <T : Serializable?> getSerializable(
        intent: Intent,
        name: String,
        clazz: Class<T>
    ): T {
        return if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU)
            intent.getSerializableExtra(name, clazz)!!
        else
            intent.getSerializableExtra(name) as T
    }
}
