package com.yeungeek.opengltutorial.data.local

import com.yeungeek.opengltutorial.data.Sample

object LocalSampleDataProvider {
    const val SAMPLE_TYPE = 1000
    const val SAMPLE_TYPE_TRIANGLE = SAMPLE_TYPE + 1
    const val SAMPLE_TYPE_COORDINATE_SYSTEMS = SAMPLE_TYPE + 2

    val sample = Sample(SAMPLE_TYPE_TRIANGLE, "Triangle", "Basic Triangle")

    val allSamples = listOf(
        Sample(SAMPLE_TYPE_TRIANGLE, "Triangle", "Basic Triangle"),
        Sample(SAMPLE_TYPE_COORDINATE_SYSTEMS, "Coordinate Systems", "Coordinate Systems"),
        Sample(3, "Triangle", "Basic Triangle"),
        Sample(4, "Coordinate Systems", "Coordinate Systems"),
        Sample(5, "Triangle", "Basic Triangle"),
        Sample(6, "Coordinate Systems", "Coordinate Systems"),
        Sample(8, "Triangle", "Basic Triangle"),
        Sample(12, "Coordinate Systems", "Coordinate Systems"),
        Sample(13, "Triangle", "Basic Triangle"),
        Sample(45, "Coordinate Systems", "Coordinate Systems"),
        Sample(46, "Triangle", "Basic Triangle"),
        Sample(64, "Coordinate Systems", "Coordinate Systems"),
        Sample(77, "Triangle", "Basic Triangle"),
        Sample(88, "Coordinate Systems", "Coordinate Systems"),
        Sample(99, "Triangle", "Basic Triangle"),
        Sample(110, "Coordinate Systems", "Coordinate Systems"),
    )
}