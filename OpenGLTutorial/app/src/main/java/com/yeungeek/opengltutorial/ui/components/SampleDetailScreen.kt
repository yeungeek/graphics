package com.yeungeek.opengltutorial.ui.components

import android.annotation.SuppressLint
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.ArrowBack
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import com.yeungeek.opengltutorial.data.Sample
import com.yeungeek.opengltutorial.data.local.LocalSampleDataProvider
import com.yeungeek.opengltutorial.renderer.ShaderRenderer
import com.yeungeek.opengltutorial.renderer.TriangleRenderer

@SuppressLint("UnusedMaterial3ScaffoldPaddingParameter")
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun SampleDetailScreen(sample: Sample, onBackPressed: () -> Unit) {
    val shaderRenderer = remember {
        ShaderRenderer(sample)
//        TriangleRenderer()
    }

    Scaffold {
        Box(modifier = Modifier.fillMaxWidth()) {
            // detail
            GLShader(
                renderer = shaderRenderer, modifier =
                Modifier
                    .fillMaxWidth()
                    .fillMaxHeight()
            )

            Box(
                modifier = Modifier
                    .height(50.dp)
                    .fillMaxWidth()
                    .background(
                        brush = Brush.verticalGradient(
                            colors = listOf(
                                Color.Black.copy(alpha = 0.3f),
                                Color.Transparent,
                            ),
                        )
                    )
            ) {
                Icon(
                    Icons.Filled.ArrowBack,
                    contentDescription = "Icon to navigate back",
                    tint = Color.White,
                    modifier = Modifier
                        .align(Alignment.TopStart)
                        .clickable {
                            onBackPressed()
                        }
                        .padding(10.dp)
                        .size(28.dp)
                )

                Text(
                    text = sample.title, modifier = Modifier.align(Alignment.Center),
                    style = MaterialTheme.typography.titleSmall,
                    fontWeight = FontWeight.Bold
                )
            }
        }
    }
}

@Preview
@Composable
fun DetailPreview() {
    val sample = LocalSampleDataProvider.sample
    SampleDetailScreen(sample) {

    }
}
