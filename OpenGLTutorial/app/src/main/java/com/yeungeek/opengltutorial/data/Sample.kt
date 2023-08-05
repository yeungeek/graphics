package com.yeungeek.opengltutorial.data

import java.io.Serializable

data class Sample(
    val id: Int,
    val title: String,
    val body: String
) : Serializable
