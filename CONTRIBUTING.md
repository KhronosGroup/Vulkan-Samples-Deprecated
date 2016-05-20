# Contributing

Contributions are encouraged! If you have a sample that demonstrates the Vulkan graphics API,
or presents an interesting use case, then consider contributing to the Vulkan samples repository.
However, before you start, check out the requirements and guidelines below. Following these
guidelines can help ensure that your contribution ends up being a valuable part of the
Vulkan samples repository.

## Repository Structure

| folder           | description                                                     |
|------------------|-----------------------------------------------------------------|
| samples/apps/    | folder with application samples, each in a separate sub-folder  |
| samples/layers/  | folder with layer samples, each in a separate sub-folder        |
| external/        | folder with commonly used external libraries                    |

## General Requirements

- Folder and description:
  - Each application sample must be placed in a separate sub-folder in the samples/apps/ folder.
  - Each layer sample must be placed in a separate sub-folder in the samples/layers/ folder.
  - Each sample should use a short folder name (all lowercase, no spaces) that best describes the sample.
  - Each sample must include a README file in the root of the sample's folder with a description.
- License:
  - Each sample is an independent project and is licensed under the LICENSE file in the sample's folder.
  - The current Contributor License Agreement (CLA) only allows samples to be licensed under the Apache 2.0 license.
  - Each sample must include a LICENSE file in the root of the sample's folder duplicating the top-level LICENSE file in the repository.
  - Every source code file must have a copyright notice and license at the top of the file as described below.
- Code and assets:
  - Each sample should be self-contained and include all source code and assets (shaders, images, geometry etc.).
  - Single source file samples with minimal build complexity are encouraged to make porting to different platforms easier.
  - Embedding SPIR-V and assets in the source code is encouraged to simplify the sample and minimize the build complexity.
  - The sample should include the source from which the SPIR-V code was generated (typically GLSL).
  - Compiling the sample with the highest warning level and warnings-as-errors (-Wall -Wextra -Werror, or /Wall /WX) is highly recommended.
- Third party libraries:
  - A sample may not depend on a separate installation of a third party library.
  - Any third party library that is used needs to be available under a compatible open source license.
  - Any third party library that is used must be included in source.
  - Commonly used third party libraries or header files may be placed in the external/ folder.

## Copyright Notice and License Template

To apply the Apache 2.0 License to your work, attach the following boilerplate notice, with
the fields enclosed by brackets "[]" replaced with your own identifying information for
the copyright year or years. *Don't include the brackets!* The text should be enclosed in the appropriate comment
syntax for the file format. We also recommend that a file or class name and description
of purpose be included on the same "printed page" as the copyright notice for easier
identification within third-party archives.

    Copyright [yyyy] [name of copyright owner]

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

## Code Style

There is no mandate for a particular code style. However, a common code style like,
for instance, the one described by the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
is recommend. A sample must consistently apply a single code style.
Changes to existing samples must follow the code style used by the sample.

When a common code style is used, or a `.clang-format` file is included with a sample,
then use `clang-format -style=` to verify the code style. For instance, use
`clang-format -style=google` to verify the Google code style, or use `clang-format -style=file`
to use the `.clang-format` file located in the closest parent folder of each input source file.

## Procedure for Contributing

  1. Fork the KhronosGroup/Vulkan-Samples repository.
  2. Add the contribution to the new fork.
  3. Make sure the above requirements are met.
  4. Make sure the sample is in compliance with the Vulkan specification.
  5. Make sure the sample code builds and runs on Windows, Linux and Android. If you cannot verify on all these target platforms, please note platform restrictions in the sample's README.
  6. Verify the sample against the Vulkan validation layers.
  7. Submit a pull request for the contribution, including electronically signing the Khronos Contributor License Agreement (CLA) for the repository using CLA-Assistant.

## Code Reviews

All submissions, including submissions by project members, are subject to a code review.
GitHub pull requests are used to facilitate the review process. A submission may be
accepted by any project member (other than the submitter), who will then squash the
changes into a single commit and cherry-pick them into the repository.

Expect every sample to be reviewed based on conformance to the Vulkan specification.
If a sample triggers false positive errors or warnings in the Vulkan validation
layers, then it is expected that these are reported as issues on the
[Vulkan-LoaderAndValidationLayers](https://github.com/KhronosGroup/Vulkan-LoaderAndValidationLayers)
repository. When filtering out a false positive in the sample, the code is expected
to have appropriate documentation and a link to the issue on the
[Vulkan-LoaderAndValidationLayers](https://github.com/KhronosGroup/Vulkan-LoaderAndValidationLayers)
repository.