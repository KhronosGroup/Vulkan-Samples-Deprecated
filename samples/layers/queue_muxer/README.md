
This layer implements queue multiplexing.

Some Vulkan drivers support only a single queue family with a single queue.
Applications that use more than one queue will not work with these Vulkan drivers.

This layer makes all Vulkan devices look like they have at least 16 queues per family.
There is virtually no impact on performance when the application only uses the queues that are exposed by the Vulkan driver.
If an application uses more queues than are exposed by the Vulkan driver,
then additional virtual queues are used that all map to the last physical queue.
When this happens there may be a noticeable impact on performance, but at least the application will work.
The impact on performance automatically goes away when new Vulkan drivers are
installed with support for more queues, even if an application continues to use this layer.
