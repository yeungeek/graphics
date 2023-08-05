package com.yeungeek.opengltutorial.ui.components

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material3.MaterialTheme
import androidx.compose.runtime.Composable
import androidx.compose.runtime.remember
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import com.yeungeek.opengltutorial.SampleUIState
import com.yeungeek.opengltutorial.data.Sample
import com.yeungeek.opengltutorial.data.local.LocalSampleDataProvider

@Composable
fun SampleListContent(
    navigateToDetail: (Sample) -> Unit = {},
    modifier: Modifier = Modifier,
) {
    val sampleUiState = remember { SampleUIState(LocalSampleDataProvider.allSamples) }
    LazyColumn(
        modifier = modifier
            .background(MaterialTheme.colorScheme.inverseOnSurface)
            .padding(top = 16.dp)
    ) {
        items(items = sampleUiState.samples, key = { it.id }) { it ->
            SampleItem(sample = it, navigateToDetail = { sample ->
                navigateToDetail(sample)
            })
        }
    }
}