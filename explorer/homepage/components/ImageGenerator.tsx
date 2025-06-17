// explorer/homepage/components/ImageGenerator.tsx
// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { Component } from '@lynx-js/react';

interface ImageGeneratorState {
  prompt: string;
  imageUrl: string | null;
  loading: boolean;
}

export default class ImageGenerator extends Component<unknown, ImageGeneratorState> {
  constructor(props: unknown) {
    super(props);
    this.state = {
      prompt: '',
      imageUrl: null,
      loading: false,
    };
  }

  handlePromptChange = (value: string) => {
    this.setState({ prompt: value });
  };

  handleGenerateImage = () => {
    // Placeholder for actual image generation logic
    if (!this.state.prompt) {
      console.log('Prompt is empty. Please enter a prompt.');
      // Optionally, update state to show a message to the user
      return;
    }

    // ---- Native Module Call Start ----
    if (global.NativeModules && global.NativeModules.ImageGeneratorModule && global.NativeModules.ImageGeneratorModule.logPrompt) {
      global.NativeModules.ImageGeneratorModule.logPrompt(this.state.prompt, (nativeMessage: string) => {
        console.log(`Callback from Native (logPrompt): ${nativeMessage}`);
      });
    } else {
      console.warn('ImageGeneratorModule or logPrompt method not found in NativeModules.');
    }
    // ---- Native Module Call End ----

    this.setState({ loading: true, imageUrl: null });
    console.log(`Requesting image with prompt: ${this.state.prompt}`);

    const placeholderApiUrl = 'https://dummyjson.com/products/add'; // Using a POST endpoint from dummyjson
    const imageDisplayUrl = `https://dummyjson.com/image/600x400/09f/fff?text=${encodeURIComponent(this.state.prompt)}`;


    try {
      // Although dummyjson.com/products/add is a POST endpoint,
      // it won't actually use our prompt to generate an image.
      // We're just practicing the POST request structure.
      const response = await fetch(placeholderApiUrl, {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({
          title: this.state.prompt, // Sending prompt as title as per dummyjson's product schema
          /* other fields if necessary for the dummy API */
        }),
      });

      // We are not parsing the response for an image URL because this dummy API
      // doesn't return one in a usable format for image generation.
      // Instead, we'll just use our pre-constructed imageDisplayUrl.
      if (response.ok) { // Check if the POST request itself was successful
        console.log('POST request successful. Simulated image URL will be used.');
        this.setState({
          imageUrl: imageDisplayUrl,
          loading: false
        });
      } else {
        console.error('Placeholder API POST request failed:', response.status, await response.text());
        // Still set the image for display purposes in this example
        this.setState({ imageUrl: imageDisplayUrl, loading: false });
      }
    } catch (error) {
      console.error('Error fetching image:', error);
      // In a real scenario, you might want to set a specific error image or message
      // For now, we'll still use the dummy image URL even on fetch error for demonstration
      this.setState({ imageUrl: imageDisplayUrl, loading: false });
    }
  };

  render() {
    return (
      <view style="display: flex; flex-direction: column; padding: 20px; background-color: #f0f0f0; height: 100%;">
        <text style="font-size: 24px; margin-bottom: 20px; text-align: center;">AI Image Generator</text>

        <input
          type="text"
          placeholder="Enter your image prompt"
          value={this.state.prompt}
          onInput={(e: any) => this.handlePromptChange(e.detail.value)}
          style="padding: 10px; margin-bottom: 15px; font-size: 16px; border: 1px solid #ccc; border-radius: 5px;"
        />

        <button
          onTap={this.handleGenerateImage}
          disabled={this.state.loading}
          style="padding: 12px 20px; background-color: #007bff; color: white; font-size: 16px; border-radius: 5px; cursor: pointer; opacity: ${this.state.loading ? 0.5 : 1};"
        >
          {this.state.loading ? 'Generating...' : 'Generate Image'}
        </button>

        {this.state.imageUrl && (
          <view style="margin-top: 20px; border: 1px solid #ddd; padding: 10px; border-radius: 5px; background-color: white; display: flex; justify-content: center; align-items: center;">
            <image
              src={this.state.imageUrl}
              style="width: 300px; height: 300px; object-fit: contain;"
              alt="Generated Image"
            />
          </view>
        )}
         {/* Lynx specific components might differ, e.g. <textinput> instead of <input type="text"> */}
         {/* The <image> tag might also have different props or require a specific aspect ratio handling */}
      </view>
    );
  }
}
