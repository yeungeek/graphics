package com.yeungeek.opengltutorial.ui.components

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material3.MaterialTheme
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import com.yeungeek.opengltutorial.SampleUIState

@Composable
fun SampleListContent(
    sampleUiState: SampleUIState,
    modifier: Modifier = Modifier,
    navigateToDetail: (Long, String) -> Unit = { _, _ -> }
) {
    LazyColumn(
        modifier = modifier
            .background(MaterialTheme.colorScheme.inverseOnSurface)
            .padding(top = 16.dp)
    ) {
        items(items = sampleUiState.samples, key = { it.id }) { sample ->
            SampleItem(sample = sample, navigateToDetail = { sampleId, sampleTitle ->
                navigateToDetail(sampleId, sampleTitle)
            })
        }
    }
}

//@Preview
//@Composable
//fun SampleListContentPreview() {
//    OpenGLTutorialTheme {
//        SampleListContent(SampleUIState(LocalSampleDataProvider.allSamples))
//    }
//}