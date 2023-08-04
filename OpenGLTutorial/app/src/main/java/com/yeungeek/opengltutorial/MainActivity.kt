package com.yeungeek.opengltutorial

import android.os.Bundle
import android.widget.Toast
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.material3.windowsizeclass.ExperimentalMaterial3WindowSizeClassApi
import androidx.compose.material3.windowsizeclass.calculateWindowSizeClass
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.tooling.preview.Preview
import com.google.accompanist.adaptive.calculateDisplayFeatures
import com.yeungeek.opengltutorial.data.local.LocalSampleDataProvider
import com.yeungeek.opengltutorial.ui.components.SampleListContent
import com.yeungeek.opengltutorial.ui.theme.OpenGLTutorialTheme

class MainActivity : ComponentActivity() {

    @OptIn(ExperimentalMaterial3WindowSizeClassApi::class)
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContent {
            OpenGLTutorialTheme {
                val windowSize = calculateWindowSizeClass(this)
                val displayFeatures = calculateDisplayFeatures(this)

                // A surface container using the 'background' color from the theme
                Surface(
                    modifier = Modifier.fillMaxSize(),
                    color = MaterialTheme.colorScheme.background
                ) {
                    SampleListContent(
                        SampleUIState(LocalSampleDataProvider.allSamples),
                        navigateToDetail = { _, sampleTitle ->
                            Toast.makeText(this, "Toast: $sampleTitle", Toast.LENGTH_SHORT).show()
                        })
                }
            }
        }
    }
}

@Preview(showBackground = true)
@Composable
fun GreetingPreview() {
    OpenGLTutorialTheme {
        SampleListContent(SampleUIState(LocalSampleDataProvider.allSamples))
    }
}