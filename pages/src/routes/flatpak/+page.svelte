<script>
	import { base } from '$app/paths';
</script>

<svelte:head>
	<title>How to use OBS Background Removal on FlatPak</title>
	<meta name="description" content="How to use OBS Background Removal on FlatPak" />
</svelte:head>

<div class="text-column">
	<h1>Download and install on FlatPak</h1>

	<p>
		<a href="https://flathub.org/apps/com.obsproject.Studio">Install the latest OBS from FlatHub</a>
	</p>

	<p>Install com.obsproject.Studio.Plugin.BackgroundRemoval from FlatHub:</p>

	<pre>flatpak install flathub com.obsproject.Studio.Plugin.BackgroundRemoval</pre>

	<p>Start your OBS</p>

	<p>Add an video capture source</p>

	<p>Open the filters dialog on that source</p>

	<p>Add the Background Removal filter on the Effects filter</p>

	<p>All set!</p>

	<p><a href="{base}/">Back to the top page</a></p>
</div>
